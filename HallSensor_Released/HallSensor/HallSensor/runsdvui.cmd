cd /d "E:\WPComponentEnablement\Huaqin\Component\HallSensor\HallSensor" &msbuild "HallSensor.vcxproj" /t:sdvViewer /p:configuration="Debug" /p:platform=ARM
exit %errorlevel% 