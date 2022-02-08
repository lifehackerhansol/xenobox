wget -O tmp.tmp http://filetrip.net/h35131406-.html
7z x -y tmp.tmp GameList.txt
rm tmp.tmp
./GameListV2Builder.rb GameList.txt
