// Offline harness around the ACTUAL product processor; no duplicated DSP chain.
#include "PluginProcessor.h"
#include <chrono>
#include <iostream>
#include <stdexcept>

namespace {
void check(bool ok, const juce::String& message) {
    if (!ok) throw std::runtime_error(message.toStdString());
}
void writeText(const juce::File& file, const juce::String& text) {
    check(file.replaceWithText(text), "Cannot write " + file.getFullPathName());
}
}

int main(int argc, char** argv) {
    try {
        check(argc == 5, "Usage: XiffectsReferenceRender input.wav output.wav config.json blockSize");
        juce::ScopedJuceInitialiser_GUI initialiser;
        const auto inputFile=juce::File::getCurrentWorkingDirectory().getChildFile(argv[1]);
        const auto outputFile=juce::File::getCurrentWorkingDirectory().getChildFile(argv[2]);
        const auto configFile=juce::File::getCurrentWorkingDirectory().getChildFile(argv[3]);
        const int blockSize=juce::String(argv[4]).getIntValue();
        check(blockSize>=1 && blockSize<=8192,"Block size must be 1..8192");
        check(inputFile!=outputFile,"Input and output must differ");
        for (const auto* suffix : {"", ".state", ".json", ".pitch.csv", ".timing.csv"})
            check(!juce::File(outputFile.getFullPathName()+suffix).exists(),"Output/sidecar exists; use a new run directory");
        juce::var config;
        check(juce::JSON::parse(configFile.loadFileAsString(),config).wasOk() && config.isObject(),"Invalid JSON config");
        const auto overrides=config.getProperty("parameters",juce::var());
        check(overrides.isObject(),"config.parameters must be an object");
        const double tail=double(config.getProperty("tailSeconds",0.25));
        check(std::isfinite(tail) && tail>=0 && tail<=10,"tailSeconds must be 0..10");

        juce::AudioFormatManager formats;formats.registerBasicFormats();
        std::unique_ptr<juce::AudioFormatReader> reader(formats.createReaderFor(inputFile));
        check(reader!=nullptr,"Cannot read input WAV");
        const double rate=reader->sampleRate;
        const int channels=int(reader->numChannels);
        check((channels==1 || channels==2) && rate>=8000 && rate<=192000,"Unsupported input format");
        check(reader->lengthInSamples>0 && reader->lengthInSamples<=juce::int64(rate*120),"Input must be 0..120 seconds");
        const int inputSamples=int(reader->lengthInSamples), totalSamples=inputSamples+int(rate*tail);
        juce::AudioBuffer<float> source(channels,totalSamples);source.clear();
        check(reader->read(&source,0,inputSamples,0,true,channels==2),"Input read failed");
        reader.reset();

        XiffectsAudioProcessor processor;
        auto layout=processor.getBusesLayout();
        const auto channelSet=channels==1?juce::AudioChannelSet::mono():juce::AudioChannelSet::stereo();
        layout.inputBuses.set(0,channelSet);layout.outputBuses.set(0,channelSet);
        check(processor.setBusesLayout(layout),"Processor rejected layout");
        for (const auto& property:overrides.getDynamicObject()->getProperties()) {
            auto* p=processor.parameters.getParameter(property.name.toString());
            check(p!=nullptr,"Unknown parameter " + property.name.toString());
            check(property.value.isInt() || property.value.isInt64() || property.value.isDouble() || property.value.isBool(),"Parameter must be numeric");
            const float value=float(property.value);
            check(std::isfinite(value) && value>=p->getNormalisableRange().start && value<=p->getNormalisableRange().end,"Parameter out of range");
            p->setValueNotifyingHost(p->convertTo0to1(value));
        }
        // Capture ALL effective values, not just overrides, before rendering.
        juce::DynamicObject::Ptr effective=new juce::DynamicObject();
        for(auto* p:processor.getParameters()) {
            const auto* ranged=dynamic_cast<juce::RangedAudioParameter*>(p);
            check(ranged!=nullptr,"Unexpected parameter type");
            effective->setProperty(ranged->paramID,ranged->convertFrom0to1(ranged->getValue()));
        }
        processor.prepareToPlay(rate,blockSize);
        juce::MemoryBlock state;processor.getStateInformation(state);
        const auto stem=outputFile.getFullPathName();
        check(outputFile.getParentDirectory().createDirectory().wasOk(),"Cannot create output directory");
        check(juce::File(stem+".state").replaceWithData(state.getData(),state.getSize()),"Cannot write state");

        juce::AudioBuffer<float> block(channels,blockSize), rendered(channels,totalSamples);
        juce::MidiBuffer midi;
        std::vector<double> timing;timing.reserve(size_t(totalSamples/blockSize+1));
        juce::String trace="frame,available_by_block_end_sample,detected_midi,applied_estimate_midi,min,max\n";
        int frameIndex=0;float peak=0;
        for(int position=0;position<totalSamples;position+=blockSize) {
            const int count=std::min(blockSize,totalSamples-position);
            for(int c=0;c<channels;++c) block.copyFrom(c,0,source,c,position,count);
            std::array<float*,2> pointers {block.getWritePointer(0),channels==2?block.getWritePointer(1):nullptr};
            juce::AudioBuffer<float> view(pointers.data(),channels,count);
            midi.clear();
            const auto begin=std::chrono::steady_clock::now();
            processor.processBlock(view,midi); // The real product chain, unmodified.
            timing.push_back(std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-begin).count());
            for(int c=0;c<channels;++c) for(int n=0;n<count;++n) {
                const float sample=view.getSample(c,n);
                check(std::isfinite(sample),"Nonfinite product output");
                peak=std::max(peak,std::abs(sample));rendered.setSample(c,position+n,sample);
            }
            xiffects::TelemetryFrame frame;
            while(processor.telemetry.pop(frame)) {
                trace+=juce::String(frameIndex++)+","+juce::String(position+count)+","+
                    juce::String(frame.detected,7)+","+juce::String(frame.corrected,7)+","+
                    juce::String(frame.minimum,7)+","+juce::String(frame.maximum,7)+"\n";
            }
        }
        juce::WavAudioFormat wav;
        std::unique_ptr<juce::FileOutputStream> stream(outputFile.createOutputStream());
        check(stream!=nullptr && stream->openedOk(),"Cannot open output");
        std::unique_ptr<juce::AudioFormatWriter> writer(wav.createWriterFor(stream.get(),rate,unsigned(channels),24,{},0));
        check(writer!=nullptr,"Cannot create WAV writer");stream.release();
        check(writer->writeFromAudioSampleBuffer(rendered,0,totalSamples),"WAV write failed");writer.reset();
        writeText(juce::File(stem+".pitch.csv"),trace);
        juce::String rawTiming="block,processBlock_wall_us\n";
        for(size_t i=0;i<timing.size();++i) rawTiming+=juce::String(int(i))+","+juce::String(timing[i],6)+"\n";
        writeText(juce::File(stem+".timing.csv"),rawTiming);
        std::sort(timing.begin(),timing.end());
        juce::DynamicObject::Ptr report=new juce::DynamicObject();
        report->setProperty("path","Reference: actual XiffectsAudioProcessor::processBlock");
        report->setProperty("measurement","offline synthetic/file rendering, NOT hardware round-trip or VST3 host validation");
        report->setProperty("input",inputFile.getFullPathName());report->setProperty("sampleRate",rate);
        report->setProperty("channels",channels);report->setProperty("blockSize",blockSize);
        report->setProperty("inputSamples",inputSamples);report->setProperty("outputSamples",totalSamples);
        report->setProperty("hostReportedLatencySamples",processor.getLatencySamples());
        report->setProperty("hardwareLatencyMeasured",false);
        report->setProperty("peak",peak);report->setProperty("effectiveParameters",juce::var(effective.get()));
        report->setProperty("processBlock_p50_us",timing[timing.size()/2]);
        report->setProperty("processBlock_p99_us",timing[(timing.size()-1)*99/100]);
        report->setProperty("processBlock_max_us",timing.back());
        writeText(juce::File(stem+".json"),juce::JSON::toString(juce::var(report.get()),true));
        processor.releaseResources();
        std::cout<<"Rendered actual Reference processor to "<<outputFile.getFullPathName()<<'\n';
        return 0;
    } catch(const std::exception& error) {
        std::cerr<<error.what()<<'\n';return 1;
    }
}
