#include <chrono>
#include <cstdint>
#include <iostream>
#include <vector>
#include <queue>
#include <map>
#include <string>
#include <functional>


#include "caf/all.hpp"

using std::endl;
using std::vector;
using std::chrono::seconds;
using namespace caf;

using std::queue;






// Each topic is a stateful actor
using topic
  = typed_actor<result<void>(put_atom, std::string),      // 'put' writes to the topic
                result<std::string>(get_atom, int32_t)>;   // 'get 'reads from the topic

struct topic_state {
  static constexpr inline const char* name = "topic";

  topic::pointer self;

  int32_t topicID;

  std::map<int32_t, queue<std::string>> state;

  topic_state(topic::pointer ptr, int32_t val) : self(ptr), topicID(val) {
    // nop
  }

	//copy prevention?
  topic_state(const topic_state&) = delete;
	//assignment copy prevention?
  topic_state& operator=(const topic_state&) = delete;

  topic::behavior_type make_behavior() {
    return {
      [=](put_atom, std::string val) { 
          // append the new message to each subscriber's queue
          for ( const auto& [key, queueCont] : state ) {
							//aout(self) << "decltype(myPair) is " << ftype_name<decltype(myPair)>() << '\n';
							
              aout(self) << "pushing message <<"<< val << ">> to queue for subscriber #" << key << endl;
              state[key].push(val); 
              //aout(self) << "front of queue is now <<" << state[key].front() <<  ">>" << endl;
          }
      },
      [=](get_atom, int32_t subscriberID) { 
          // check if this subscriber already exists. If not, create a subscription (queue)
          if ( state.find(subscriberID) == state.end() ) {
						state[subscriberID];
            //state[subscriberID] = new queue<std::string>;
            return std::string("");  // no message when subscribing
          } else {
            if (state[subscriberID].empty()) {
							aout(self) << "fetching head of queue for subscriber #" << subscriberID << ", but it was empty" << endl;
							return std::string("");  // no message available
						}
						std::string x = state[subscriberID].front(); 
						
						aout(self) << "fetching head of queue for subscriber #" << subscriberID << ", the message is: <<" << x << ">>" << endl;
						state[subscriberID].pop(); 
						return x;
          }
      },
    };
  }
};

using topic_impl = topic::stateful_impl<topic_state>;


void publisher(event_based_actor* self, std::string message, vector<topic> topics) {
  for (auto& x : topics)
    self->request(x, caf::infinite, put_atom_v, message);
}

void blocking_publisher(blocking_actor* self, std::string message, vector<topic> topics) {
  for (auto& x : topics)
    self->request(x, caf::infinite, put_atom_v, message);
}

void fetch(blocking_actor* self, int32_t subscriberID, vector<topic> topics, std::function<void(std::string)> consumer_function) {
  for (auto& x : topics)
    self->request(x, caf::infinite, get_atom_v, subscriberID)
      .receive(
        [&](std::string y) {
					if(y.compare(std::string(""))  != 0) {
						//aout(self) << "msg is: <" << y << ">" <<  endl;
						consumer_function(y);
					}
        },
        [&](error& err) {
          aout(self) << "topic #" << x.id() << " -> " << to_string(err) << endl;
        });
}

void non_blocking_fetch(event_based_actor* self, int32_t subscriberID, vector<topic> topics, std::function<void(std::string)> consumer_function) {
  for (auto& x : topics)
    self->request(x, caf::infinite, get_atom_v, subscriberID).then(
			[=](std::string y) {
				if(y.compare(std::string("")) != 0) {
					//aout(self) << "msg is: <" << y << ">" <<  endl;
					consumer_function(y);
				}
			});
}
#include <thread>
// Showcase
void caf_main(actor_system& system) {

  // vector of 5 different topics
  vector<topic> topics;
  for (int32_t i = 0; i < 5; ++i)
    topics.emplace_back(system.spawn<topic_impl>(i));
  scoped_actor self{system};

  // create four subscribers (they each subscribe to some topics...)
  // fetching a topic creates a subscription for this subscriber if not yet subscribed
	auto sub1_topics = vector<topic>(1, topics[0]);
  system.spawn(non_blocking_fetch, 1, sub1_topics, [=](std::string s){});
	
	auto sub2_topics = vector<topic>(1, topics[1]);
	system.spawn(non_blocking_fetch, 2, sub2_topics, [&](std::string s){});
	auto sub3_topics = vector<topic>(1, topics[2]);
	system.spawn(non_blocking_fetch, 3, sub3_topics, [&](std::string s){});
	system.spawn(non_blocking_fetch, 4, topics, [&](std::string s){});

	for(int i = 0; i < 9999999; i++) //wait
		3+4;
  // publish "hey there" to each topic
  aout(self) << "publisher publishing messages" << endl;
  system.spawn(publisher, "hey there to all", topics);
	system.spawn(publisher, "this is only for topic 1", sub1_topics);
	system.spawn(publisher, "topic 3 exclusive", sub3_topics);

  // subscribers read from each topic
  
	
	for(int i = 0; i < 99999999; i++) //wait
		3+4;
	
	aout(self) << "subscribers fetching messages even though they may not have arrived" << endl;
	/*
	
	system.spawn(fetch, 1, sub1_topics, [&](std::string s){aout(self) << "Subscriber 1  consuming data: <<<<<" << s << ">>>>> " << std::endl;});
  system.spawn(fetch, 2, sub2_topics, [&](std::string s){aout(self) << "Subscriber 2  consuming data: <<<<<" << s << ">>>>> " << std::endl;});
  system.spawn(fetch, 3, sub3_topics, [&](std::string s){aout(self) << "Subscriber 3  consuming data: <<<<<" << s << ">>>>> " << std::endl;});
	
  system.spawn(fetch, 4, topics, [&](std::string s){aout(self) << "Subscriber 4 peaking at all data, consuming data: <<<<<" << s << ">>>>>" << std::endl;});
	*/
	
	
  system.spawn(non_blocking_fetch, 1, sub1_topics, [&](std::string s){aout(self) << "Subscriber 1 consuming data: <<<<<" << s << ">>>>> " << std::endl;});
  system.spawn(non_blocking_fetch, 2, sub2_topics, [&](std::string s){aout(self) << "Subscriber 2 consuming data: <<<<<" << s << ">>>>> " << std::endl;});
  system.spawn(non_blocking_fetch, 3, sub3_topics, [&](std::string s){aout(self) << "Subscriber 3 consuming data: <<<<<" << s << ">>>>> " << std::endl;});
	
  system.spawn(non_blocking_fetch, 4, topics, [&](std::string s){aout(self) << "Subscriber 4 peaking at all data, consuming data: <<<<<" << s << ">>>>> " << std::endl;});
	
	system.spawn(non_blocking_fetch, 1, sub1_topics, [&](std::string s){aout(self) << "Subscriber 1 consuming data: <<<<<" << s << ">>>>> " << std::endl;});
	system.spawn(non_blocking_fetch, 2, sub2_topics, [&](std::string s){aout(self) << "Subscriber 2 consuming data: <<<<<" << s << ">>>>> " << std::endl;});
  system.spawn(non_blocking_fetch, 3, sub3_topics, [&](std::string s){aout(self) << "Subscriber 3 consuming data: <<<<<" << s << ">>>>> " << std::endl;});
	system.spawn(non_blocking_fetch, 4, topics, [&](std::string s){aout(self) << "Subscriber 4 peaking at all data, consuming data: <<<<<" << s << ">>>>> " << std::endl;});
	
}
// --(rst-main-end)--

CAF_MAIN()
