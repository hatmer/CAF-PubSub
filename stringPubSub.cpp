#include <chrono>
#include <cstdint>
#include <iostream>
#include <vector>
#include <queue>
#include <map>
#include <string>

#include "ftypename.hpp"

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
							
              aout(self) << "pushing message "<< val << " to queue for subscriber #" << key << endl;
              state[key].push(val); 
              aout(self) << "front of queue is now " << state[key].front() << endl;
          }
      },
      [=](get_atom, int32_t subscriberID) { 
          // check if this subscriber already exists. If not, create a subscription (queue)
          if ( state.find(subscriberID) == state.end() ) {
						state[subscriberID];
            //state[subscriberID] = new queue<std::string>;
            return std::string("");  // no message when subscribing
          } else {
						aout(self) << "fetching head of queue for subscriber #" << subscriberID << endl;
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
// --(rst-ddtopic-end)--



// blocking functions that call receive on each topic in vector: correspond to publisher and subscriber

void publisher(blocking_actor* self, std::string message, vector<topic> topics) {
  for (auto& x : topics)
    self->request(x, seconds(1), put_atom_v, message);
}

void fetch(blocking_actor* self, int32_t subscriberID, vector<topic> topics) {
  for (auto& x : topics)
    self->request(x, seconds(1), get_atom_v, subscriberID)
      .receive(
        [&](std::string y) {
          aout(self) << "subscriber #" << subscriberID << " received message <" << y << ">" << endl;
        },
        [&](error& err) {
          aout(self) << "topic #" << x.id() << " -> " << to_string(err) << endl;
        });
}

void non_blocking_fetch(event_based_actor* self, int32_t subscriberID, vector<topic> topics) {
  for (auto& x : topics)
    self->request(x, seconds(1), get_atom_v, subscriberID).await(
			[=](std::string y) {
				aout(self) << "subscriber #" << subscriberID << " received message <" << y << ">" << endl;
			});
}




// testing
void caf_main(actor_system& system) {
	std::cout << "decltype(hej text) is " << ftype_name<decltype("hej")>() << '\n';
	std::string s = "hehe";
	std::cout << "decltype(hehe text) is " << ftype_name<decltype(s)>() << '\n';
	
	
  // vector of 5 different topics
  vector<topic> topics;
  for (int32_t i = 0; i < 5; ++i)
    topics.emplace_back(system.spawn<topic_impl>(i));
  scoped_actor self{system};

  // create three subscribers (they each subscribe to all topics...)
  // fetching a topic creates a subscription for this subscriber if not yet subscribed
  system.spawn(fetch, 1, topics);
  //system.spawn(fetch, 2, topics);
  //system.spawn(fetch, 3, topics);
	
	//system.spawn(non_blocking_fetch, 4, topics);
	//system.spawn(non_blocking_fetch, 5, topics);
	system.spawn(non_blocking_fetch, 6, topics);

  // publish "5" to each topic
  aout(self) << "publisher publishing messages" << endl;
  system.spawn(publisher, "hey there", topics);

  // subscribers read from each topic
  aout(self) << "subscribers fetching messages" << endl;
  system.spawn(fetch, 1, topics);
  //system.spawn(fetch, 2, topics);
  //system.spawn(fetch, 3, topics);
	
	//system.spawn(non_blocking_fetch, 4, topics);
	//system.spawn(non_blocking_fetch, 5, topics);
	system.spawn(non_blocking_fetch, 6, topics);
  
}
// --(rst-main-end)--

CAF_MAIN()
