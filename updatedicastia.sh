wget --no-check-certificate -O tmp.tmp https://sites.google.com/site/dicaztia/Dicastia_AP_Patch_v0.2.zip
7z x -y tmp.tmp Dicastia.txt
rm tmp.tmp
./GameListV2Builder.rb Dicastia.txt
