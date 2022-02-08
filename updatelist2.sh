wget -O tmp.tmp http://files-ds-scene.net/romtool/releases/databases/offsets.dsapdb
xenobox dsapfilt d < tmp.tmp > GameList.txt
rm tmp.tmp
./GameListV2Builder.rb GameList.txt
