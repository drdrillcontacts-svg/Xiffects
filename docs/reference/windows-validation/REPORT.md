# اعتبارسنجی Windows / Reference — گزارش نهایی این مرحله

## وضعیت خلاصه

**بیلد Windows x64 Release، اجرای واقعی ReferenceRender و آزمون بستهٔ VST3 در میزبان اعتبارسنجی pluginval انجام و موفق شدند.** این نتیجه به معنی تأیید کیفیت تجاری، آزمون تعاملی REAPER یا عملکرد کنسرت نیست. هیچ Live Experimental ساخته نشده است.

- سورس موتور/رابط مرجع: `26e8bb36aac808f1922373e65e0c8a357cf668df`.
- commit ابزارهای اجرای موفق: `5eba0d8`؛ در ابتدای CI، hash تمام Source/Assets و diff نسبت به Reference بررسی و تأیید شد.
- اجرای Windows: https://github.com/drdrillcontacts-svg/Xiffects/actions/runs/39678399287
- artifact شامل VST3/Standalone، WAVها، trace، timing، state، پارامترهای مؤثر، گزارش JSON، گزارش Build/CTest/pluginval و CMakeCache:
  https://github.com/drdrillcontacts-svg/Xiffects/actions/runs/39678399287/artifacts/12383840216
- بازتحلیل مستقل همان WAVها و traceهای ذخیره‌شده، بدون اجرای دوبارهٔ DSP:
  https://github.com/drdrillcontacts-svg/Xiffects/actions/runs/39679964500
- بازتحلیل، تمام ۴۸ نتیجهٔ محاسبه‌شده را با خروجی اولیه یکسان یافت. تست مستقل FFT و خواندن PCM24 نیز روی Windows پاس شد.

## ۱. مشکل محیط محلی

علت دقیق در `PKGCONFIG.md` و stdout/stderr ثبت شده است: executable مربوط به pkg-config وجود ندارد؛ هدرهای ALSA/X11/FreeType نیز موجود نیستند. CMake از کشف وابستگی‌های Linux در JUCE درخواست PkgConfig می‌کند. این محیط به مخازن معمول بسته‌های Debian دسترسی مجاز ندارد و بستهٔ cached هم موجود نیست.

در نتیجه Linux محلی با روش جعلی مانند حذف dependency check یا دادن executable صوری «رفع» نشده است. راه‌حل اصولی، نصب بسته‌های native روی محیط دارای دسترسی است. Windows CI مسیر واقعی جایگزین برای اعتبارسنجی Windows بود، نه ادعای تعمیر Linux.

اولین اجرای workflow در بررسی byte-hash و پیش از configure متوقف شد. تنظیم `core.autocrlf=false` پیش از checkout، بررسی دقیق بایت‌های Reference را در اجرای بعد موفق کرد؛ Source تغییر نکرد.

## ۲. نتایج تست و دامنهٔ آن‌ها

| تست | نتیجه | توضیح |
|---|---|---|
| Configure و Build Windows x64 Release | PASS | محصول، renderer و تست‌های native ساخته شدند |
| DSP | PASS | CTest، حدود 0.080 s |
| Pitch | PASS | CTest، حدود 1.244 s |
| CorrectionRegression | PASS | CTest، حدود 0.124 s |
| Realtime | PASS | CTest، حدود 0.035 s؛ allocation audit مستقل، نه گواهی real-time کل میزبان |
| Host | PASS | CTest، حدود 0.455 s؛ state، POWER-off bypass، مونو/استریو، بلوک متغیر و GUI smoke |
| ReferenceProcessor | PASS | CTest، حدود 0.602 s؛ مسیر فعال واقعی محصول، استقلال بلوک‌بندی و state-render round trip |
| FixtureTests | PASS | قالب و تکرارپذیری ورودی‌های مصنوعی |
| کیفیت انتخاب نزدیک‌ترین نت در onset ورودی 452 Hz | FAIL نسبت به معیار نزدیک‌ترین نت | خروجی حدود 466 Hz به‌جای 440 Hz؛ جزئیات پایین |
| ReferenceRender | PASS | ۴۸ فایل از همان `XiffectsAudioProcessor::processBlock`، نه زنجیرهٔ کپی‌شده |
| pluginval 1.0.4 / strictness 5 | PASS، exit 0 | بارگذاری بستهٔ واقعی VST3 در میزبان اعتبارسنجی مستقل |
| FFT/PCM24 analyser tests | PASS | دو unit test محلی و روی Windows، در اجرای review |
| بازتحلیل artifact | PASS | نتایج هر ۴۸ WAV/trace با اندازه‌گیری اولیه یکسان بود |
| REAPER / DAW تعاملی | NOT RUN | در این مرحله جلسهٔ DAW ایجاد و آزمایش نشده است |
| ورودی/خروجی فیزیکی و ASIO loopback | NOT RUN | تأخیر سخت‌افزاری یا dropout واقعی دستگاه سنجیده نشده |
| ضبط واقعی آواز و ساکسوفون | NOT RUN | فایل مجاز واقعی هنوز موجود نیست؛ harmonic proxy ضبط واقعی نیست |

`CTestResults.json` وضعیت تک‌تک CTestها و `pluginval-coverage.txt` بخش قابل‌بازیابی پوشش میزبان را نگه می‌دارند. لاگ کامل داخل artifact است.

### پوشش واقعی میزبان VST3

pluginval بستهٔ `Xiffects.vst3` را بارگذاری و تست‌های Audio processing، Plugin state و Automatable Parameters را اجرا کرده است. لاگ، پردازش در 44.1/48/96 kHz و بافرهای 64/128/256/512/1024، همچنین sub-blockهای 32 نمونه را نشان می‌دهد.

این میزبان validation واقعی است اما DAW اجرایی REAPER نیست. تست POWER/bypass مستقیم در HostTests پاس شده؛ bypass تعاملی در REAPER، انتخاب دستگاه، ورودی میکروفون و شنیدن خروجی کارت صدا انجام نشده‌اند. موفقیت pluginval تأیید کیفیت کوک یا طبیعی‌بودن صدا نیست.

عبور workflow به معنی موفقیت Build/تست‌های کدنویسی/اجرای ابزار است. تحلیل‌گر کیفیت را
اندازه‌گیری و گزارش می‌کند و فعلاً برای آستانهٔ خطای correction یا deadline، CI را fail
نمی‌کند. بنابراین نتیجهٔ سبز CI نباید به معنی پاس‌شدن معیار کیفیت nearest-note تفسیر شود.

## ۳. نتایج صوت واقعی خروجی محصول روی ورودی مصنوعی

تنظیمات: correction روشن، Strength=100، Mix=100، Retune=12 ms، Chromatic، همهٔ نت‌ها مجاز؛ سایر افکت‌ها bypass. با وجود bypass، کد محصول همچنان واحدهای مربوط را طبق Reference پردازش می‌کند. بافر رندر ۶۴ نمونه؛ WAVها در سه نرخ 44.1/48/96 kHz تولید شدند.

تحلیل مستقل خروجی از FFT با جست‌وجوی گستردهٔ 50–4000 Hz استفاده می‌کند؛ به اطراف نت مورد انتظار محدود نیست. «فرکانس غالب» لزوماً fundamental ادراکی یک سیگنال پیچیده نیست. برای glide/vibrato/step، خطای یک فرکانس ثابت خروجی گزارش نشده است.

### یافتهٔ مهم: قفل‌شدن روی نت مجاور پس از onset

ورودی 452 Hz با 100 ms سکوت اولیه و onset پنج‌میلی‌ثانیه‌ای، باید در آزمون «نزدیک‌ترین نت کروماتیک» به A4=440 Hz برسد. اما خروجی اندازه‌گیری‌شده چنین بود:

| نرخ | خروجی غالب WAV | خطا نسبت به 440 Hz | میانهٔ خطای تشخیص ورودی |
|---|---:|---:|---:|
| 44.1 kHz | 466.100 Hz | +99.762 سنت | 0.239 سنت |
| 48 kHz | 466.053 Hz | +99.588 سنت | 0.423 سنت |
| 96 kHz | 466.040 Hz | +99.541 سنت | 0.425 سنت |

این **خطای اکتاو نیست**؛ انتخاب نت مجاور A#/Bb4 است. trace واقعی، اولین تشخیص MIDI=69.514 در 44.1 kHz و حدود 69.587 در 48/96 kHz را نشان می‌دهد، سپس منحنی applied در MIDI≈70 باقی می‌ماند. رفتار با hysteresis فعلی و عبور تخمین اولیه از مرز نیم‌نت سازگار است؛ شواهد در `OnsetDiagnostics.json` ثبت شده‌اند.

تست سادهٔ قبلی بدون این onset، این مورد را آشکار نکرده بود. این یافته ثبت شده و **اصلاح نشده** است؛ اجازهٔ تغییر موتور در این مرحله وجود نداشت. بنابراین عبور تست‌های قبلی به معنی دقت همیشگی correction نیست.

### سایر یافته‌ها

- در فریم‌های voiced مقایسه‌شده، خطای اکتاو با آستانهٔ حداقل ۶۰۰ سنت مشاهده نشد. این ادعا فقط دربارهٔ فریم‌های شمرده‌شدهٔ همین corpus است، نه تمام ورودی‌ها یا نت‌های خارج از محدوده.
- در 48 kHz میانهٔ خطای tracking ورودی glide حدود **39.243 سنت** و vibrato حدود **23.261 سنت** بود. مقایسه با truth همان زمانِ انتهای بلوک انجام شده و تأخیر تحلیل جبران نشده است؛ این اعداد صرفاً خطای steady-state detector نیستند.
- هیچ نمونهٔ خروجی با قدر مطلق ≥0.999 در این رندرها ثبت نشد؛ خروجی nonfinite نیز باعث شکست renderer می‌شد و مشاهده نشد.
- برای سکوت/noise، تشخیص voiced اشتباه در بازهٔ مورد بررسی دیده نشد. فیلد unvoiced در آخر نت‌های دارای Pitch می‌تواند به نزدیک‌ترین label مرزی 1.1 ثانیه اشاره کند؛ نباید آن را بدون بررسی مرز، false positive داخل سکوت تفسیر کرد.
- بررسی max adjacent-sample jump و peak در گزارش خام موجود است؛ اندازه‌گیری jump به‌تنهایی اثبات کلیک/پاپ نیست. ارزیابی شنیداری و آزمون ضبط واقعی هنوز لازم است.

## ۴. زمان‌بندی و تأخیر — بدون ادعای سخت‌افزاری

| نرخ، بافر 64 | بیشترین p99 بین fixtureها | بیشترین زمان بلوک | دفعات عبور از مهلت اسمی |
|---|---:|---:|---:|
| 44.1 kHz | 114.100 µs | 288.8 µs | 0 |
| 48 kHz | 143.524 µs | 1930.2 µs | 2 |
| 96 kHz | 92.256 µs | 896.7 µs | 1 |

دو عبور در `sine_880_48000` و یک عبور در `harmonic_sax_proxy_96000` دیده شد. این‌ها wall time اجرای offline در runner اشتراکی هستند، **نه سه dropout تأییدشدهٔ کارت صدا**؛ scheduler/preemption هم می‌تواند مؤثر باشد. بااین‌حال نتیجه اجازهٔ تضمین real-time/کنسرت نمی‌دهد. آخرین بلوک کوتاه در شمارش deadline لحاظ نشده است.

در ورودی 452 Hz، اولین تشخیص در محدودهٔ ۲۵ سنت از Pitch معلوم، حداکثر تا 1478/1344/2688 نمونه پس از onset در 44.1/48/96 kHz در دسترس بود. timestamp از انتهای بلوک است، onset با سکوت قبلی شروع شده و این مقدار با startup از حالت buffer خالی یکسان نیست. این **زمان تشخیص** است، نه زمان عبور صدای محصول.

`hostReportedLatencySamples` مقدار اعلامی processor است؛ HostTests عبور impulse در مسیر POWER-off را با آن مقایسه کرده و پاس شده است. ابزار مستقل قبلی هم تأخیر اجزای دو شِیفتر را سنجیده بود. هیچ‌کدام معادل اندازه‌گیری ADC→DAC نیستند. **هیچ عددی به‌عنوان تأخیر سخت‌افزاری ورودی تا خروجی محصول اعلام نمی‌شود.**

## ۵. بازتولید

Windows x64، Visual Studio C++ و CMake؛ از همان سورس Reference و ابزارهای این مرحله:

```powershell
git config --global core.autocrlf false
cmake -S . -B build-reference-win -A x64 -DXIFFECTS_BUILD_REFERENCE_TOOLS=ON
cmake --build build-reference-win --config Release --parallel 2
ctest --test-dir build-reference-win -C Release -V --output-junit evidence/ctest.xml
python -m pip install numpy==2.2.6
python Tools/GenerateReferenceFixtures.py build-reference-fixtures
python Tests/FixtureTests.py
python Tests/ReferenceAnalysisTests.py
python Tools/ValidateReferenceWindows.py --renderer build-reference-win/Release/XiffectsReferenceRender.exe --fixtures build-reference-fixtures --output evidence/renders
pluginval.exe --strictness-level 5 --validate build-reference-win/Xiffects_artefacts/Release/VST3/Xiffects.vst3
```

پوشهٔ خروجی renderer باید تازه باشد؛ فایل‌های قبلی عمداً overwrite نمی‌شوند. pluginval از release رسمی v1.0.4 دریافت شد و SHA-256 ZIP ابزار در artifact ذخیره است. دستور `core.autocrlf` باید پیش از checkout تازه اجرا شود و تنظیم جهانی Git محیط آزمون را تغییر می‌دهد، نه سورس موتور را.

برای بازتحلیل فایل‌های artifact دانلودشده:

```powershell
python Tools/ReviewReferenceEvidence.py downloaded reviewed-new
```

دستورها و ترتیب دقیق در دو workflow جدید، جزئیات runner در artifact و وضعیت API اجراها در همین پوشه ذخیره‌اند. دانلود مستقیم artifact در sandbox به‌علت مسدودبودن Azure Blob انجام نشد؛ برای دسترسی و بازتحلیل، runner Windows دوم artifact را دانلود و نتیجه را از API GitHub منتشر کرد. فایل‌های WAV و لاگ کامل از لینک artifact قابل دریافت‌اند، نه از فایل‌های خلاصهٔ محلی.

annotation اولیهٔ metrics در GitHub به 4096 کاراکتر بریده شد؛ آن متن ناقص مبنای آمار نهایی نیست. اجرای review نتایج را در chunkهای کوچک منتشر کرد و `measured-results.json` شامل تمام ۴۸ ردیف بازیابی‌شده است. داده‌های خام با دقت کامل در artifact باقی مانده‌اند؛ اعداد جدول محلی برای خوانایی گرد شده‌اند.

## ۶. تغییرات و موارد باز

**تغییرات فقط ابزار/CI/مستندات:**
- `.github/workflows/reference-windows.yml`: build، CTest، fixture، renderer و pluginval واقعی Windows.
- `.github/workflows/reference-evidence-review.yml`: بازتحلیل artifact و تست تحلیل‌گر.
- `Tools/ValidateReferenceWindows.py`: اجرای corpus از processor و تحلیل مستقل خروجی.
- `Tools/ReviewReferenceEvidence.py`: بررسی دوبارهٔ WAVها/traceها و استخراج گزارش میزبان.
- `Tests/ReferenceAnalysisTests.py`: تست FFT گسترده و PCM24.
- `docs/reference/windows-validation/`: خطاها، وضعیت‌ها، شواهد API، metrics و گزارش.

Source، Assets، CMake محصول، ReferenceRender موجود، الگوریتم‌ها و مسیر DSP تغییر نکرده‌اند. اختلاف Source/Assets با `26e8bb3` خالی و hashها منطبق‌اند.

بازمانده‌ها: مشکل نت مجاور در onset، آزمون شنیداری، اجرای تعاملی DAW، ورودی واقعی آواز/ساکسوفون، آزمون طولانی با audio device و loopback. اصلاح هیچ‌کدام در این مرحله بدون بررسی تو آغاز نمی‌شود. Reference حفظ شده و Live Experimental آغاز نشده است.
