COLORS=("Black " "Red   " "Green " "Brown " "Blue  " "Violet" "Cyan  " "Gray  ")
LINE=""
for FG in {0..7}
do
	LINE=" ${LINE} | \033[3${FG}m${COLORS[FG]}"
done

echo -e "\033[0m$LINE"
echo -e "\033[1m$LINE"


#echo -e "${BLACK}BLACK${RED}RED${GREEN}GREEN${BROWN}BROWN${BLUE}BLUE"