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


template <class T>
using template_topic
  = typed_actor<
								result<void>(put_atom, T),      // 'put' writes to the topic
								result<T>(get_atom, int32_t)>;   // 'get 'reads from the topic
								
// Each topic is a stateful actor
// using topic
  // = typed_actor<result<void>(put_atom, std::string),      // 'put' writes to the topic
                // result<std::string>(get_atom, int32_t)>;   // 'get 'reads from the topic

template <class T>
struct topic_state {
  static constexpr inline const char* name = "template_topic";
	
	class
  template_topic<T>::pointer self;

  int32_t topicID;

  std::map<int32_t, queue<T>> state;

  topic_state(class template_topic<T>::pointer ptr, int32_t val) : self(ptr), topicID(val) {
    // nop
  }

	//copy prevention?
  topic_state(const topic_state&) = delete;
	//assignment copy prevention?
  topic_state& operator=(const topic_state&) = delete;

  class template_topic<T>::behavior_type make_behavior() {
    return {
      [=](put_atom, T val) { 
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
            return T();  // no message when subscribing
          } else {
						aout(self) << "fetching head of queue for subscriber #" << subscriberID << endl;
            if (state[subscriberID].empty())
              return T();  // no message available
						T x = state[subscriberID].front(); 
						state[subscriberID].pop(); 
						return x;
          }
      },
    };
  }
};

template <class T>
using topic_impl = class template_topic<T>::template stateful_impl<template topic_state<T>>;

// template <class T>
// using topic_impl = class typed_actor<
								// result<void>(put_atom, T),
								// result<T>(get_atom, int32_t)>::template stateful_impl<topic_state<T>>;
								
								
//using topic_impl = topic::stateful_impl<topic_state>;
// --(rst-ddtopic-end)--



// blocking functions that call receive on each topic in vector: correspond to publisher and subscriber
template <class T>
void publisher(blocking_actor* self, T message, vector<template_topic<T>> topics) {
  for (auto& x : topics)
    self->request(x, seconds(1), put_atom_v, message);
}

template <class T>
void non_blocking_fetch(event_based_actor* self, int32_t subscriberID, vector<template_topic<T>> topics) {
  for (auto& x : topics)
    self->request(x, seconds(1), get_atom_v, subscriberID).await(
			[=](T y) {
				aout(self) << "subscriber #" << subscriberID << " received message <" << y << ">" << endl;
			});
}




// testing
void caf_main(actor_system& system) {
  // vector of 5 different topics
  vector<template_topic<std::string>> topics;
  for (int32_t i = 0; i < 5; ++i)
    topics.emplace_back(system.spawn<topic_impl<std::string>>(i));
  scoped_actor self{system};

  // create three subscribers (they each subscribe to all topics...)
  // fetching a topic creates a subscription for this subscriber if not yet subscribed

	system.spawn(non_blocking_fetch, 6, topics);

  // publish "5" to each topic
  aout(self) << "publisher publishing messages" << endl;
  system.spawn(publisher, "hey there", topics);

  // subscribers read from each topic
  aout(self) << "subscribers fetching messages" << endl;
	system.spawn(non_blocking_fetch, 6, topics);
  
}
// --(rst-main-end)--

CAF_MAIN()
