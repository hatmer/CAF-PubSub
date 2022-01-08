/******************************************************************************\
 * Illustrates semantics of request().{then|await|receive}.                   *
\******************************************************************************/

#include <chrono>
#include <cstdint>
#include <iostream>
#include <vector>
#include <queue>
#include <map>
#include <string>

#include "caf/all.hpp"

using std::endl;
using std::vector;
using std::chrono::seconds;
using namespace caf;

using std::string;
using std::queue;

// Each topic is a stateful actor
using topic
  = typed_actor<result<void>(put_atom, int32_t),      // 'put' writes to the topic
                result<int32_t>(get_atom, int32_t)>;   // 'get 'reads from the topic

struct topic_state {
  static constexpr inline const char* name = "topic";

  topic::pointer self;

  int32_t topicID;

  std::map<int32_t, queue<int32_t>*> state;

  topic_state(topic::pointer ptr, int32_t val) : self(ptr), topicID(val) {
    // nop
  }

  topic_state(const topic_state&) = delete;

  topic_state& operator=(const topic_state&) = delete;

  topic::behavior_type make_behavior() {
    return {
      [=](put_atom, int32_t val) { 
          // append the new message to each subscriber's queue
          for ( const auto &myPair : state ) {
              aout(self) << "pushing value "<< val << " to queue for subscriber #" << myPair.first << endl;
              state[myPair.first]->push(val); 
              aout(self) << "front of queue is now " << state[myPair.first]->front() << endl;
          }
      },
      [=](get_atom, int32_t subscriberID) { 
          // check if this subscriber already exists. If not, create a subscription (queue)
          if ( state.find(subscriberID) == state.end() ) {
            state[subscriberID] = new queue<int32_t>;
            return -1;  // no message when subscribing
          } else {
						aout(self) << "fetching head of queue for subscriber #" << subscriberID << endl;
            if (state[subscriberID]->empty())
              return -1;  // no message available
						int32_t x = state[subscriberID]->front(); 
						state[subscriberID]->pop(); 
						return x;
          }
      },
    };
  }
};

using topic_impl = topic::stateful_impl<topic_state>;
// --(rst-ddtopic-end)--



// blocking functions that call receive on each topic in vector: correspond to publisher and subscriber

void publisher(blocking_actor* self, int32_t message, vector<topic> topics) {
  for (auto& x : topics)
    self->request(x, seconds(1), put_atom_v, message);
}

void fetch(blocking_actor* self, int32_t subscriberID, vector<topic> topics) {
  for (auto& x : topics)
    self->request(x, seconds(1), get_atom_v, subscriberID)
      .receive(
        [&](int32_t y) {
          aout(self) << "subscriber #" << subscriberID << " received message <" << y << ">" << endl;
        },
        [&](error& err) {
          aout(self) << "topic #" << x.id() << " -> " << to_string(err) << endl;
        });    
}


// testing
void caf_main(actor_system& system) {
  // vector of 5 different topics
  vector<topic> topics;
  for (int32_t i = 0; i < 5; ++i)
    topics.emplace_back(system.spawn<topic_impl>(i));
  scoped_actor self{system};

  // create three subscribers (they each subscribe to all topics...)
  // fetching a topic creates a subscription for this subscriber if not yet subscribed
  system.spawn(fetch, 1, topics);
  system.spawn(fetch, 2, topics);
  system.spawn(fetch, 3, topics);

  // publish "5" to each topic
  aout(self) << "publisher publishing messages" << endl;
  system.spawn(publisher, 5, topics);

  // subscribers read from each topic
  aout(self) << "subscribers fetching messages" << endl;
  system.spawn(fetch, 1, topics);
  system.spawn(fetch, 2, topics);
  system.spawn(fetch, 3, topics);
  
}
// --(rst-main-end)--

CAF_MAIN()
