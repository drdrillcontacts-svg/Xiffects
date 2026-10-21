# Reference — مبنای ثبت‌شده، بدون تغییر موتور

## وضعیت تحویل

**Reference سورس ثبت شده؛ اعتبارسنجی تازهٔ Windows/VST3 و اجرای renderer واقعی هنوز انجام نشده‌اند.**
هیچ Live Experimental ساخته نشده و هیچ رفتار DSP، مسیر صوتی، شناسهٔ پارامتر یا رابط کاربری تغییر نکرده است.

### نقطهٔ بازگشت

- commit کامل: `26e8bb36aac808f1922373e65e0c8a357cf668df`
- نام منطقی مسیر: **Reference**
- hash فایل‌های اولیه: `baseline-files.json`؛ شامل Source، Assets، تست‌ها، ابزارها، CMake و workflow اولیه.
- بایگانی مستقل سورس: `Xiffects-Reference-26e8bb3.zip`؛ اندازه و SHA-256 در `source-archive.json`.
- commit مرجع **پیش از** افزودن renderer، ورودی‌ساز و ابزار جمع‌آوری شواهد ساخته شد. ابزارها در تغییرات جدا هستند.
- commit محلی است؛ در این مرحله push، تغییر شاخه، force-push یا جایگزینی فایل‌های موتور انجام نشده است.

بازسازی بایگانی، بدون تغییر شاخه یا دست‌زدن به فایل‌های کاری:

```sh
git archive --format=zip --output=Reference-source.zip 26e8bb36aac808f1922373e65e0c8a357cf668df
git diff 26e8bb36aac808f1922373e65e0c8a357cf668df -- Source Assets
```

خروجی دستور دوم در این مرحله خالی است. ZIP سورس، JUCE دانلودشده یا باینری پلاگین را شامل نمی‌شود.

## نتایج اجراشده و اجرا‌نشده

| بررسی | وضعیت این مرحله | محدودهٔ ادعا |
|---|---|---|
| CMake و build مستقل Release | موفق | بدون wrapper/GUI و وابستگی native JUCE |
| DSP | PASS | الگوریتم‌های مستقل |
| Pitch | PASS | ورودی مصنوعی؛ طیف نزدیک فرکانس موردانتظار، correction و latency جزء نرم‌افزاری |
| CorrectionRegression | PASS | مرز تشخیص و برابری کنترل اصلاح در اتومیشن |
| Realtime | PASS | حلقهٔ مستقل، تخصیص معمول heap و ترتیب صف؛ نه کل processBlock در DAW |
| FixtureTests | PASS | تکرارپذیری فایل و label، قالب WAV و hash؛ نه کیفیت پردازش صوتی |
| تولید ورودی مصنوعی | انجام شد | ۴۸ WAV: ۱۶ نوع × 44.1/48/96 kHz، به‌همراه truth ورودی |
| PitchProfile موجود | اجرا شد | زیرسیستم مستقل Pitch، نه زنجیرهٔ کامل محصول |
| configure native | FAILED | `Could NOT find PkgConfig`؛ متن خطا ثبت شده است |
| ReferenceRender جدید | NOT RUN / BUILD UNVERIFIED | لینک به محصول طراحی شده، اما در این محیط کامپایل/اجرا تأیید نشده |
| ReferenceProcessor جدید و Host موجود | NOT RUN | نیازمند بیلد native JUCE |
| Windows VST3/Standalone این snapshot | NOT RUN | بیلد تاریخی پروژه جای این آزمون نیست |
| پلاگین در DAW / pluginval | NOT RUN | تست ابزار مستقل به معنی تست VST3 نیست |
| آواز واقعی / ساکسوفون واقعی | NOT RUN | ضبط دارای مجوز در دسترس نیست |
| تأخیر ورودی تا خروجی سخت‌افزاری | NOT MEASURED | هیچ loopback یا ورودی زنده اجرا نشده |

### خروجی خام و ابزارها

تمام دستورات، مسیر اجرا، exit code و نام فایل stdout/stderr در
`runs/local-01/commands.json` ثبت شده‌اند. خروجی verbose هر تست جدا نگهداری شده است.
`execution-status.json` اجرا‌نشده‌ها را جدا مشخص می‌کند؛ `environment.json`، خروجی نسخهٔ compiler/CMake/Python، commit/tag حل‌شدهٔ JUCE و هر دو `CMakeCache.txt` نیز موجودند.

در `source-integrity.json` تطابق تمام فایل‌های Source و Assets با Reference ثبت شده است.
نام «PASS» کنار اجرای ابزار فقط موفقیت همان دستور است، نه تأیید کیفیت تجاری محصول.

## تفکیک اندازه‌گیری‌ها

این اعداد از اجرای واقعی ابزار **مستقل و مصنوعی نرم‌افزاری** به دست آمده‌اند:

| نرخ نمونه‌برداری | فاصلهٔ impulse در دو شِیفتر unity | اولین تشخیص معتبر سینوس‌های آزمون |
|---|---:|---:|
| 44.1 kHz | 2650 نمونه | 1536 نمونه |
| 48 kHz | 2884 نمونه | 2048 نمونه |
| 96 kHz | 5764 نمونه | 4096 نمونه |

- ستون دوم اندازه‌گیری داخلی دو جزء نرم‌افزاری است؛ نه اندازه‌گیری جدید کل PluginProcessor یا رفت‌وبرگشت محصول.
- ستون سوم acquisition آشکارساز از حالت reset است، نه تأخیر عبور صوت؛ به ستون دوم جمع نمی‌شود.
- زمان بلوک‌ها در خروجی خام profile موجود است؛ شامل wrapper، شش افکت دیگر، GUI، درایور و سخت‌افزار نیست. آزمون کوتاه، تضمین نبود dropout نیست.
- **هیچ عددی به‌عنوان تأخیر واقعی ورودی تا خروجی محصول اعلام نمی‌شود.** اهداف Live و کیفیت هنوز هدف‌اند و مسیر جدیدی برای اندازه‌گیری وجود ندارد.

## ابزار ReferenceRender

`Tools/ReferenceRender.cpp` به کتابخانهٔ خود `Xiffects` لینک می‌شود و مستقیماً:

1. WAV مونو/استریو و تنظیمات JSON را می‌خواند؛ هیچ resampling پنهانی انجام نمی‌دهد.
2. همان `XiffectsAudioProcessor` را می‌سازد و پارامترهای واقعی APVTS را تنظیم می‌کند.
3. `prepareToPlay` و سپس **`processBlock` واقعی** را روی بلوک‌ها اجرا می‌کند.
4. WAV خروجی، تمام پارامترهای مؤثر، state باینری، telemetry و زمان خام هر بلوک را ذخیره می‌کند.

پردازش الگوریتمی دیگری در این ابزار کپی نشده است. تخصیص حافظه و I/O مربوط به harness خارج از فراخوانی زمان‌گیری‌شدهٔ processBlock انجام می‌شوند. اجرا offline است، نه تست thread با اولویت real-time. خروجی و sidecarهای موجود بازنویسی نمی‌شوند.

`hostReportedLatencySamples` صرفاً مقدار اعلامی processor است؛ اندازه‌گیری سخت‌افزاری نیست.
ستون `available_by_block_end_sample` در trace زمان در‌دسترس‌بودن رکورد تا انتهای بلوک است، نه timestamp دقیق آشکارساز. منحنی applied، همان تخمین موتور است و نباید با تحلیل مستقل صوت خروجی اشتباه گرفته شود.

`Tests/ReferenceTests.cpp` نیز خود processor را می‌سازد و برای مونو/استریو در سه نرخ، زنجیرهٔ فعال را با بلوک‌بندی متفاوت و state بازیابی‌شده مقایسه می‌کند. این تست **افزوده شده اما اجرا نشده** است.

## ورودی‌ها و حقوق استفاده

تولیدکنندهٔ `Tools/GenerateReferenceFixtures.py` ورودی‌های silence، noise، سینوس‌های معلوم، نت خارج از محدوده، detune، تغییر نت، glide، vibrato و دو harmonic proxy می‌سازد.

- `manifest.json` تولیدشده شامل SHA-256 WAV، نرخ، طول و truth فرکانس ورودی در فواصل زمانی است.
- truth، فرکانس تحلیلی ورودی است؛ نه نتیجهٔ آشکارساز یا truth صوت اصلاح‌شده.
- در `runs/local-01/synthetic-fixtures.json` خلاصهٔ پایدار corpus و hash manifest ذخیره شده است.
- فایل‌های تولیدی زیر `build-reference-fixtures/` هستند و وارد Git نمی‌شوند؛ با دستور زیر بازتولید می‌شوند.
- دو harmonic proxy **ضبط آواز یا ساکسوفون نیستند**. فایل‌های واقعی دو دسته با manifest مستقل و وضعیت «فایل مجاز موجود نیست» در `datasets/voice/` و `datasets/saxophone/` ثبت شده‌اند.
- هیچ فایل عمومی بدون بررسی مجوز دانلود نشده است. قبل از ورود ضبط واقعی، منبع، صاحب حق، مجوز، اجازهٔ آزمون/توزیع و منشأ annotation ثبت می‌شوند.

corpus تازه هنوز از renderer واقعی عبور نکرده است؛ نتایج تست‌های مستقل را به آن نسبت نمی‌دهیم. استخراج شاخص‌های جامع خطای اکتاو/کیفیت ضبط واقعی از وظایف مرحلهٔ مقایسه است.

## دستورات اجرای مستقل در این محیط

```sh
python Tools/CaptureReference.py --cmake .venv/bin/cmake --output docs/reference/runs/local-02
```

پوشهٔ خروجی باید جدید باشد. خروج موفق ابزار جمع‌آوری به معنی موفقیت آزمون‌های مستقل است؛ وضعیت native و موارد اجرا‌نشده را باید در گزارش خواند.

دستورات اصلی، مستقل از ابزار جمع‌آوری:

```sh
cmake -S . -B build-reference-portable -DXIFFECTS_DSP_ONLY=ON -DXIFFECTS_BUILD_BENCHMARKS=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build-reference-portable --parallel 2
ctest --test-dir build-reference-portable -V
./build-reference-portable/XiffectsPitchProfile
python Tools/GenerateReferenceFixtures.py build-reference-fixtures
python Tests/FixtureTests.py
```

در اجرای ثبت‌شده، CMake/CTest از `.venv/bin` در PATH و Python سیستم استفاده کردند؛ دستورات دقیق در commands.json هستند. اجرای هر CTest با `-R` نیز جدا ثبت شده است.

## دستورهای Windows — آمادهٔ اجرا، نه نتیجهٔ تأییدشده

در Developer PowerShell با Visual Studio 2022 Desktop C++، CMake، Git و Python:

```powershell
cmake -S . -B build-reference-win -A x64 -DXIFFECTS_BUILD_REFERENCE_TOOLS=ON
cmake --build build-reference-win --config Release --parallel 2
ctest --test-dir build-reference-win -C Release -V
python Tools/GenerateReferenceFixtures.py build-reference-fixtures
python Tests/FixtureTests.py
build-reference-win/Release/XiffectsReferenceRender.exe build-reference-fixtures/detuned_452_48000.wav build-reference-output/corrected.wav docs/reference/configs/correction.json 64
```

خروجی اصلی پلاگین زیر `build-reference-win/Xiffects_artefacts/Release/VST3/` قرار می‌گیرد. پس از build واقعی، باید در DAW نیز تست شود؛ ReferenceRender فقط processor را اجرا می‌کند، نه بسته‌بندی VST3 و رفتار میزبان.

گزینهٔ Reference tools پیش‌فرض خاموش است؛ workflow قبلی بدون تغییر مانده و خودبه‌خود این ابزار اختیاری را تأیید نمی‌کند.

## فایل‌های این مرحله

- ثبت baseline: commit مرجع، `baseline-files.json`، وضعیت اولیهٔ Git و فایل‌های ثبت commit/archive.
- تغییرات build/testing فقط: `CMakeLists.txt` برای targetهای اختیاری؛ `.gitignore` برای cache پایتون.
- جدید: `Tools/ReferenceRender.cpp`، `Tools/GenerateReferenceFixtures.py`، `Tools/CaptureReference.py`.
- جدید: `Tests/ReferenceTests.cpp`، `Tests/FixtureTests.py`.
- جدید: گزارش‌ها، configهای bypass/correction، manifestهای خالی دادهٔ واقعی، raw logs، وضعیت و hash شواهد در `docs/reference/`.
- **Source، Assets، تست‌های قبلی، ابزار PitchProfile قبلی و workflow تغییر نکرده‌اند.**

پس از بررسی این گزارش و اجرای native/Windows، می‌توان دربارهٔ مرحلهٔ مقایسه تصمیم گرفت. این مرحله مجوز حذف Reference یا شروع جایگزینی با Live نیست.
