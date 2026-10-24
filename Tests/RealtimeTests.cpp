#include "DSP/Pitch.h"
#include "DSP/LivePitchBank.h"
#include "Telemetry.h"
#include "DSP/ReverbBank.h"
#include <atomic>
#include <cstdlib>
#include <iostream>
#include <new>
#include <thread>

std::atomic<bool> audit{false};
std::atomic<size_t> allocations{0};
void* operator new(std::size_t bytes) {
    if(audit.load()) ++allocations;
    if(void* result=std::malloc(bytes?bytes:1)) return result;
    throw std::bad_alloc();
}
void operator delete(void* memory) noexcept {std::free(memory);}
void operator delete(void* memory,std::size_t) noexcept {std::free(memory);}
void* operator new[](std::size_t bytes) {return ::operator new(bytes);}
void operator delete[](void* memory) noexcept {::operator delete(memory);}
void operator delete[](void* memory,std::size_t) noexcept {::operator delete(memory);}
void require(bool result,const char* message) {
    if(!result) {std::cerr<<message<<std::endl;std::exit(1);}
}
int main() {
    using namespace xiffects;
    static_assert(std::atomic<float>::is_always_lock_free,"Audio telemetry requires lock-free float atomics");
    static_assert(std::atomic<size_t>::is_always_lock_free,"Audio queue requires lock-free index atomics");
    Compressor comp;comp.prepare(48000);Delay delay;delay.prepare(48000);
    Distortion dist;dist.prepare(48000);Equalizer eq;eq.prepare(48000);
    Phaser phaser;phaser.prepare(48000);Reverb reverb;reverb.prepare(48000);
    ReverbBank bank;bank.prepare(48000);
    PitchShifter shift;shift.prepare(48000);PitchCorrection tune;tune.prepare(48000);
    LivePitchBank live;live.prepare(48000);
    std::array<bool,12> notes;notes.fill(true);
    TelemetryQueue queue;
    audit.store(true);
    float peak=0;
    for(int i=0;i<48000;++i) {
        const float in=0.1f*std::sin(2*pi*452*float(i)/48000);
        const float ratio=tune.process(in,0,1,notes,1,12);
        auto x=shift.process({in,in},ratio,1,tune.detector.frequency);
        x=live.process(x,ratio,1,0.5f,2.f,0.2f,0.2f,tune.detector.frequency);
        const float gain=comp.gain(std::abs(x[0]),-18,4,3);
        for(size_t c=0;c<2;++c) x[c]=eq.process(dist.process(x[c]*gain,c,0.2f,0.5f),c,{1,1,1,1});
        x=phaser.process(x,0.6f,0.4f,0.35f);x=delay.process(x,0.25f,0.5f);
        for(size_t c=0;c<2;++c) x[c]=reverb.process(x[c],c,0.5f,2);
        x=bank.process(x,.5f,2,(i/128)%4);
        peak=std::max(peak,std::abs(x[0]));
        queue.push({tune.detected,tune.corrected,x[0],x[1]});
        TelemetryFrame frame;queue.pop(frame);
    }
    audit.store(false);
    require(allocations.load()==0,"DSP processing must not allocate");
    require(std::isfinite(peak),"Finite integrated DSP output");
    std::atomic<bool> ordered{true};
    std::thread producer([&] {
        for(int i=0;i<100000;++i)
            while(!queue.push({float(i),float(i),-float(i),float(i)})) std::this_thread::yield();
    });
    for(int i=0;i<100000;++i) {
        TelemetryFrame frame;
        while(!queue.pop(frame)) std::this_thread::yield();
        if(frame.detected!=float(i)||frame.minimum!=-float(i)) ordered.store(false);
    }
    producer.join();require(ordered.load(),"SPSC telemetry ordering under concurrent wraparound");
    std::cout<<"Zero DSP allocations; concurrent telemetry test passed\n";
}
