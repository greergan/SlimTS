#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>
#include <slim/common/log.h>
#include <slim/queue/queue.h>

#ifdef HAVE_LIBRDKAFKA
#include <librdkafka/rdkafkacpp.h>
#endif

namespace slim::queue {
	using namespace slim::common;
	std::mutex queue_mutex;
	static std::unordered_map<std::string, std::vector<job*>> queues;
	static std::atomic<long> ticket_id(0);
	static const unsigned int sleep_time = 1;  // next try with std::conditional_variable
	// refactor to use std::condition_variable::wait()
}
slim::queue::job::job(){}
slim::queue::job::job(std::string queue_name_string, std::string source_storage_handle, std::string source_file_name_string)
		: queue_name_string(queue_name_string) {
	ingress_job_file.storage_container_handle = source_storage_handle;
	ingress_job_file.file_name_string = source_file_name_string;
}
long slim::queue::job::get_ticket_id() {
	return ticket_id;
}
void slim::queue::job::set_ticket_id(long new_ticket_id) {
	this->ticket_id = new_ticket_id;
}
slim::queue::job* slim::queue::get_job(std::string queue_name_string) {
	log::trace(log::Message("slim::queue::get_job()",std::string("begins, queue name => " + queue_name_string).c_str(),__FILE__, __LINE__));
	job* returning_job_object;
	std::atomic<bool> job_found = false;
	enter_for_loop:
	std::unique_lock<std::mutex> lock(queue_mutex);
	for(auto* job_object : queues[queue_name_string]) {
		if(job_object->ready_to_process && !job_object->submitted) {
			std::string answer_string = job_object->ready_to_process ? "true" : "false";
			log::debug(log::Message("slim::queue::get_job()","job " + job_object->ingress_job_file.file_name_string + " ready => " + answer_string,__FILE__, __LINE__));
			job_object->set_ticket_id(++ticket_id);
			job_object->submitted = true;
			returning_job_object = job_object;
			job_found = true;
			break;
		}
	}
	lock.unlock();
	if(!job_found) {
		std::this_thread::sleep_for(std::chrono::milliseconds(sleep_time));
		goto enter_for_loop;
	}
	log::trace(log::Message("slim::queue::get_job()",std::string("ends, queue name => " + queue_name_string).c_str(),__FILE__, __LINE__));
	return returning_job_object;
}
slim::queue::job* slim::queue::find_job(std::string queue_name_string, long job_ticket_id) {
	log::trace(log::Message("slim::queue::find_job()","begins queue name => " + queue_name_string + ", ticket id => " + std::to_string(job_ticket_id),__FILE__, __LINE__));
	slim::queue::job* returning_job_object;
	std::unique_lock<std::mutex> lock(queue_mutex);
	for(auto* job_object : queues[queue_name_string]) {
		if(job_object->get_ticket_id() == job_ticket_id) {
			returning_job_object = job_object;
		}
	}
	auto answer_string = returning_job_object ? "true" : "false";
	log::trace(log::Message("slim::queue::find_job()","ends queue name => " + queue_name_string
		+ ", ticket id => " + std::to_string(job_ticket_id) + " job found => " + answer_string,__FILE__, __LINE__));
	return returning_job_object;
}
void slim::queue::submit(job* job_object) {
	log::trace(log::Message("slim::queue::submit()","begins, queue name => " + job_object->queue_name_string + ", job name => " + job_object->ingress_job_file.file_name_string,__FILE__, __LINE__));
	std::unique_lock<std::mutex> lock(queue_mutex);
	queues[job_object->queue_name_string].emplace_back(job_object);
	job_object->ready_to_process = true;
	lock.unlock();
	for(;;) {
		lock.lock();
		if(!job_object->completed) {
			lock.unlock();
			std::this_thread::sleep_for(std::chrono::milliseconds(sleep_time));
		}
		else {
			lock.unlock();
			break;
		}
		
	}
	lock.lock();
	for(std::vector<job*>::iterator job_iterator = queues[job_object->queue_name_string].begin(); job_iterator != queues[job_object->queue_name_string].end();) {
		job* temp_job = *job_iterator;
		job_object->get_ticket_id() == temp_job->get_ticket_id() ? queues[job_object->queue_name_string].erase(job_iterator) : job_iterator++;
	}
log::trace(log::Message("slim::queue::submit()","ends, queue name => " + job_object->queue_name_string + ", job name => " + job_object->ingress_job_file.file_name_string,__FILE__, __LINE__));
}