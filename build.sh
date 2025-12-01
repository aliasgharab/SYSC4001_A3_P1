if [ ! -d "bin" ]; then
    mkdir bin
else
	rm bin/*
fi

g++ -g -O0 -I . -o bin/interrupts_EP interrupts_AliBundookwalla_MohamedGomaa_EP.cpp
g++ -g -O0 -I . -o bin/interrupts_RR interrupts_AliBundookwalla_MohamedGomaa_RR.cpp
g++ -g -O0 -I . -o bin/interrupts_EP_RR interrupts_AliBundookwalla_MohamedGomaa_EP_RR.cpp