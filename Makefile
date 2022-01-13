

pull:
	g++ -g stringPubSub.cpp -std=c++17 -lcaf_core -lcaf_io -o pull_main.out && ./pull_main.out
	
push:
	g++ -g stringPubSubPush.cpp -std=c++17 -lcaf_core -lcaf_io -o push_main.out && ./push_main.out

test:
	g++ -g pubsub.cpp -std=c++17 -lcaf_core -lcaf_io -o main.out && ./main.out
	
clean:
	rm main.out
