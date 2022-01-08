	
test:
	g++ -g pubsub.cpp -std=c++17 -lcaf_core -lcaf_io -o main.out && ./main.out

clean:
	rm main.out
