#int TOPICS = 1;
#int SUBS = 1000;
#int PUBS_LOOPS = 100;
#int MSG_SIZE = 100;

test:
	g++ -g stringPubSub.cpp -std=c++17 -lcaf_core -lcaf_io -o main.out
	g++ -g stringPubSubPush.cpp -std=c++17 -lcaf_core -lcaf_io -o mainPush.out
		
testop1 = -1-10-1000-100
testop2 = -1-100-1000-100
testop5 = -10-100-1000-100
testop6 = -10-500-1000-100

testop3 = -10-100-10000-100
testop4 = -10-500-10000-100
testing:
#g++ -g stringPubSub$(testop1).cpp -std=c++17 -lcaf_core -lcaf_io -o main$(testop1).out

#g++ -g stringPubSub$(testop2).cpp -std=c++17 -lcaf_core -lcaf_io -o main$(testop2).out

#	g++ -g stringPubSub$(testop3).cpp -std=c++17 -lcaf_core -lcaf_io -o main$(testop3).out

#	g++ -g stringPubSub$(testop4).cpp -std=c++17 -lcaf_core -lcaf_io -o main$(testop4).out

	g++ -g stringPubSub$(testop5).cpp -std=c++17 -lcaf_core -lcaf_io -o main$(testop5).out

	g++ -g stringPubSub$(testop6).cpp -std=c++17 -lcaf_core -lcaf_io -o main$(testop6).out

testingPush:
	g++ -g stringPubSubPush$(testop3).cpp -std=c++17 -lcaf_core -lcaf_io -o mainPush$(testop3).out

	g++ -g stringPubSubPush$(testop4).cpp -std=c++17 -lcaf_core -lcaf_io -o mainPush$(testop4).out

clean:
	rm main*.out
