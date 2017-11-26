gcc -g program.c -o program -pthread -Wall &&
gnome-terminal --command="./program 1" &&
gnome-terminal --command="./program 2" &&
gnome-terminal --command="./program 3" &&
gnome-terminal --command="./program 4" 