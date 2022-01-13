#include <chrono>
#include <cstdint>
#include <iostream>
#include <vector>
#include <queue>
#include <map>
#include <string>
#include <functional>
#include <cassert>

#include "ftypename.hpp"

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

		std::map<int32_t, queue<std::string>> state; //to be removed
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
						aout(self) << "pushing message <"<< val << "> to subscribers. " << endl;
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


	// blocking functions that call receive on each topic in vector: correspond to publisher and subscriber

	void publisher(blocking_actor* self, std::string message, vector<topic> topics) {
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
			self->request(top, caf::infinite, get_atom_v).then(
				[=](int32_t topic_id) {
						//self->state.subscriber_id = sub_id;
						aout(self) << "subscription of topic <" << topic_id << "> started" << endl;
				}
		);
		
		
		return {
				[=](put_atom, std::string topic_message, int32_t topic_id) {
					aout(self) << "subscriber of topic <" << topic_id << "> received a message: <" << topic_message << ">, performing subscriber function on message..." << endl;
					consumer_function(topic_message);
				} //exit fun;
		};
	}

	void unordered_subscriber() {
		
	}

	/*
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
	*/



	// testing
	
}

void caf_main(actor_system& system) {
		using namespace pusher;
		// std::cout << "decltype(hej text) is " << ftype_name<decltype("hej")>() << '\n';
		// std::string s = "hehe";
		// std::cout << "decltype(hehe text) is " << ftype_name<decltype(s)>() << '\n';
		
		
		// vector of 5 different topics
		vector<topic> topics;
		for (int32_t i = 0; i < 5; ++i)
			topics.emplace_back(system.spawn<topic_impl>(i));
		scoped_actor self{system};
		//std::cout << "decltype(self{system}) is " << ftype_name<decltype(self)>() << '\n';
		// create three subscribers (they each subscribe to all topics...)
		// fetching a topic creates a subscription for this subscriber if not yet subscribed
		//system.spawn(fetch, 1, topics);
		auto sub = system.spawn(ordered_subscriber, topics, [](std::string s){std::cout << s << std::endl;});
		
		//system.spawn(non_blocking_fetch, 6, topics);

		// publish "hey" to each topic
		aout(self) << "publisher publishing messages" << endl;
		system.spawn(publisher, "hey there", topics);
		
		//std::cout << "decltype(sub) is " << ftype_name<decltype(sub)>() << '\n';
		
		aout(self) << "publisher publishing messages" << endl;
		system.spawn(publisher, "hello again", topics);

		// subscribers read from each topic
		aout(self) << "subscribers fetching messages" << endl;
		//system.spawn(fetch, 1, topics);
		//system.spawn(fetch, 2, topics);
		//system.spawn(fetch, 3, topics);
		
		//system.spawn(non_blocking_fetch, 4, topics);
		//system.spawn(non_blocking_fetch, 5, topics);
		//system.spawn(non_blocking_fetch, 6, topics);
		
	}
	// --(rst-main-end)--

CAF_MAIN()