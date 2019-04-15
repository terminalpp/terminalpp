# Takes the fonts bundled with the terminal and installs them on linux.
# we simply copy the fonts into the ~/.fonts folder and recreate the font cache as described here:
# https://en.wikibooks.org/wiki/Guide_to_X11/Fonts

mkdir -p ~/.fonts
cp tpp/fonts/* ~/.fonts/
fc-cache -v
