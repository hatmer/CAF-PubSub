#include <chrono>
#include <cstdint>
#include <iostream>
#include <vector>
#include <queue>
#include <map>
#include <string>
#include <functional>
#include <cassert>


#include "caf/all.hpp"

using std::endl;
using std::vector;
using std::chrono::seconds;
using namespace caf;

using std::queue;


namespace pusher {
	using subber = typed_actor<
		result<void>(put_atom, std::string, int32_t)>;

	// Each topic is a stateful actor
	using topic
		= typed_actor<result<void>(put_atom, std::string),       // push message to subscribers
									result<int32_t>(get_atom)>;								 // a new subscriber subscibes

	struct topic_state {
		static constexpr inline const char* name = "topic";

		topic::pointer self;

		int32_t topicID;
		int32_t subscriber_id_offset = 0; //wrap around to be implemented mb

		std::map<int32_t, subber> subscribers; //weak pointer

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
						aout(self) << "topic #" << topicID << " pushing message <"<< val << "> to subscribers" << endl;
						for ( const auto& [key, sub] : subscribers ) {
								
								//aout(self) << "decltype(myPair) is " << ftype_name<decltype(myPair)>() << '\n';
								self->send(sub, put_atom_v, val, topicID);
						}
				},
				[=](get_atom) { //subscriber subscribes first time
							auto sender = self->current_sender();
							auto senderHandle = caf::actor_cast<subber>(sender);
							//std::cout << "current_sender is: " << ftype_name<decltype(sender)>() << std::endl;
							subscriber_id_offset++;
							subscribers[subscriber_id_offset] = senderHandle;
							
							//self->monitor sender perhaps
							return topicID;  // return subId
					 
				}
			};
		}
	};

	using topic_impl = topic::stateful_impl<topic_state>;


	// blocking functions that call receive on each topic in vector: correspond to publisher and subscriber

	void publisher(event_based_actor* self, std::string message, vector<topic> topics) {
		for (auto& top : topics)
			self->request(top, caf::infinite, put_atom_v, message);
	}

	struct subscriber_state { //Guessing this isnt needed
		static inline const char* name = "subscriber";
		int32_t subscriber_id = 0; //useless?
		//consumer function?
	};

	subber::behavior_type ordered_subscriber(subber::stateful_pointer<subscriber_state> self, vector<topic> topics, std::function<void(std::string)> consumer_function) {
		for (auto& top : topics)
			self->request(top, caf::infinite, get_atom_v).await(
				[=](int32_t topic_id) {
						//self->state.subscriber_id = sub_id;
						aout(self) << "subscription of topic <" << topic_id << "> started" << endl;
				}
		);
		
		
		return {
				[=](put_atom, std::string topic_message, int32_t topic_id) {
				
					aout(self) << "subscriber of topic <" << topic_id << "> received a message: <" << topic_message << ">, performing subscriber function on message...(currently empty)" << endl;
					consumer_function(topic_message);
				} //exit fun;
		};
	}
	
}

void caf_main(actor_system& system) {
		using namespace pusher;
		// vector of 5 different topics
  vector<topic> topics;
  for (int32_t i = 0; i < 5; ++i)
    topics.emplace_back(system.spawn<topic_impl>(i));
  scoped_actor self{system};

  // create four subscribers (they each subscribe to some topics...)
  // fetching a topic creates a subscription for this subscriber if not yet subscribed
	auto sub1_topics = vector<topic>(1, topics[0]);
  system.spawn(ordered_subscriber, sub1_topics, [=](std::string s){});
	
	auto sub2_topics = vector<topic>(1, topics[1]);
	system.spawn(ordered_subscriber, sub2_topics, [&](std::string s){});
	auto sub3_topics = vector<topic>(1, topics[2]);
	system.spawn(ordered_subscriber, sub3_topics, [&](std::string s){});
	system.spawn(ordered_subscriber, topics, [&](std::string s){});

	for(int i = 0; i < 9999999; i++) //wait
		3+4;
  // publish "hey there" to each topic
  aout(self) << "publisher publishing messages" << endl;
  system.spawn(publisher, "hey there to all", topics);
	system.spawn(publisher, "this is only for topic 0", sub1_topics);
	system.spawn(publisher, "topic 2 exclusive", sub3_topics);

  // subscribers read from each topic
  
	system.spawn(publisher, "Late publication to all topics", topics);
		
	}
	// --(rst-main-end)--

CAF_MAIN()