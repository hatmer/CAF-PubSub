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
              state[key].push(val); 
          }
      },
      [=](get_atom, int32_t subscriberID) { 
          // check if this subscriber already exists. If not, create a subscription (queue)
          if ( state.find(subscriberID) == state.end() ) {
						state[subscriberID];
            return std::string("");  // no message when subscribing
          } else {
            if (state[subscriberID].empty())
              return std::string("");  // no message available
						std::string x = state[subscriberID].front(); 
						state[subscriberID].pop(); 
						return x;
          }
      },
    };
  }
};

using topic_impl = topic::stateful_impl<topic_state>;



// blocking functions that call receive on each topic in vector: correspond to publisher and subscriber
/*
void publisher(blocking_actor* self, std::string message, vector<topic> topics) {
  for (auto& x : topics)
    self->request(x, caf::infinite, put_atom_v, message);
}*/

void publisher(event_based_actor* self, std::string message, vector<topic> topics) {
  for (auto& x : topics)
    self->request(x, caf::infinite, put_atom_v, message);
}

void fetch(blocking_actor* self, int32_t subscriberID, vector<topic> topics, std::function<void(std::string)> consumer_function) {
  for (auto& x : topics)
    self->request(x, caf::infinite, get_atom_v, subscriberID)
      .receive(
        [&](std::string y) {
					consumer_function(y);
        },
        [&](error& err) {
          aout(self) << "topic #" << x.id() << " -> " << to_string(err) << endl;
        });
}

void non_blocking_fetch(event_based_actor* self, int32_t subscriberID, vector<topic> topics, std::function<void(std::string)> consumer_function) {
  for (auto& x : topics)
    self->request(x, caf::infinite, get_atom_v, subscriberID).await(
			[=](std::string y) {
				consumer_function(y);
			});
}

void publisher_loop(event_based_actor* self, vector<topic> topics, int max, int msg_size) {
	std::string msg = std::string(msg_size, 's');
	for(int i = 0; i < max; i++) {
		self->spawn(publisher, msg, topics);
	}
}

void subscriber_loop(event_based_actor* self, int i, vector<topic> topics, std::function<void(std::string)> consumer_function, int max) {
	for(int i = 0; i < max; i++) {
		self->spawn(non_blocking_fetch, i, topics, consumer_function);
	}
}

void caf_main(actor_system& system) {
		int TOPICS = 10;
		int SUBS = 500;
		int PUBS_LOOPS = 1000 / 100;
		int MSG_SIZE = 100;
		
		vector<topic> topics;
		for (int32_t i = 0; i < TOPICS; ++i)
			topics.emplace_back(system.spawn<topic_impl>(i));
		scoped_actor self{system};
		
		
		for(int i = 0; i < SUBS; i++) {
			auto subActor = system.spawn(non_blocking_fetch, i, topics, [=](std::string s){/*aout(self) << s << std::endl*/}); //subscribe
		}

		// publish "hey" to each topic
		aout(self) << "publisher publishing messages" << endl;
		
		//std::string s;
		for(int i = 0; i < PUBS_LOOPS; i++)
			system.spawn(publisher_loop, topics, 100, MSG_SIZE);
		
		aout(self) << "subscribers reading messages" << endl;
		for(int j = 0; j < PUBS_LOOPS; j++)
			for(int i = 0; i < SUBS; i++) { 
				system.spawn(subscriber_loop, i, topics, [&](std::string s){/*aout(self) << s << std::endl;*/}, 100);
				//auto subActor = system.spawn(non_blocking_fetch, i, topics, [&](std::string s){aout(self) << s << std::endl;}); //consume messages
			}
		
	}

CAF_MAIN()
