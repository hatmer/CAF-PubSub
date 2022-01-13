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
						//aout(self) << "pushing message <"<< val << "> to subscribers. " << endl;
						for ( const auto& [key, sub] : subscribers ) {
								//aout(self) << "decltype(myPair) is " << ftype_name<decltype(myPair)>() << '\n';
								//return (put_atom_v, val);
								self->send(sub, put_atom_v, val, topicID);
								//aout(self) << "front of queue is now " << state[key].front() << endl;
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
						//aout(self) << "subscription of topic <" << topic_id << "> started" << endl;
				}
		);
		
		
		return {
				[=](put_atom, std::string topic_message, int32_t topic_id) {
					//aout(self) << "subscriber of topic <" << topic_id << "> received a message: <" << topic_message << ">, performing subscriber function on message..." << endl;
					consumer_function(topic_message);
				} //exit fun;
		};
	}



	// testing
	
}

void publisher_loop(event_based_actor* self, vector<pusher::topic> topics, int max, int msg_size) {
	std::string msg = std::string(msg_size, 's');
	for(int i = 0; i < max; i++) {
		//for (auto& top : topics)
		self->spawn(pusher::publisher, msg, topics);
			//self->request(top, caf::infinite, put_atom_v, message);
	}
}

void caf_main(actor_system& system) {
		using namespace pusher;
		int TOPICS = 10;
		int SUBS = 100;
		int PUBS_LOOPS = 10000 / 100;
		int MSG_SIZE = 100;
		
		vector<topic> topics;
		for (int32_t i = 0; i < TOPICS; ++i)
			topics.emplace_back(system.spawn<topic_impl>(i));
		scoped_actor self{system};
		
		
		for(int i = 0; i < SUBS; i++)
			auto sub = system.spawn(ordered_subscriber, topics, [=](std::string s){/*aout(self) << s << std::endl*/});

		// publish "hey" to each topic
		aout(self) << "publisher publishing messages" << endl;
		
		//std::string s;
		for(int i = 0; i < PUBS_LOOPS; i++)
			system.spawn(publisher_loop, topics, 100, MSG_SIZE);
		
	}

CAF_MAIN()