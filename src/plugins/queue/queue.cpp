#include <future>
#include <mutex>
#include <string>
#include <slim/common/log.h>
#include <slim/plugin.hpp>
#include <slim/queue/queue.h>
namespace slim::plugin::queue {
	using namespace slim;
	using namespace slim::common;
	void listen(const v8::FunctionCallbackInfo<v8::Value>& args);
	namespace job {
		void complete(const v8::FunctionCallbackInfo<v8::Value>& args);
	}
}
void slim::plugin::queue::job::complete(const v8::FunctionCallbackInfo<v8::Value>& args) {
	log::trace(log::Message("slim::plugin::queue::job::complete()", "begins",__FILE__, __LINE__));
	auto* isolate = args.GetIsolate();
	auto context = isolate->GetCurrentContext();
	v8::HandleScope handle_scope(isolate);
	auto job_object = args[0].As<v8::Object>();
	auto v8_ticket_id = job_object->Get(context, utilities::StringToV8String(isolate, "ticket_id")).ToLocalChecked();
	auto v8_queue_name_string = job_object->Get(context, utilities::StringToV8String(isolate, "queue_name_string")).ToLocalChecked();
	auto ticket_id = utilities::V8ValueToInt(isolate, v8_ticket_id);
	auto queue_name_string = utilities::v8ValueToString(isolate, v8_queue_name_string);
	log::debug(log::Message("slim::plugin::queue::job::complete()",std::string("job.ticket_id = " + std::to_string(ticket_id)).c_str(),__FILE__,__LINE__));
	log::debug(log::Message("slim::plugin::queue::job::complete()",std::string("queue name = " + queue_name_string).c_str(),__FILE__,__LINE__));
	auto* job = slim::queue::find_job(queue_name_string, ticket_id);
	if(job) {
		auto v8_egress_job_file_object = job_object->Get(context, utilities::StringToV8String(isolate, "egress_job_file")).ToLocalChecked().As<v8::Object>();
		auto v8_file_name_string = v8_egress_job_file_object->Get(context, utilities::StringToV8String(isolate, "file_name_string")).ToLocalChecked();
		auto v8_storage_container_handle = v8_egress_job_file_object->Get(context, utilities::StringToV8String(isolate, "storage_container_handle")).ToLocalChecked();
		std::unique_lock<std::mutex> lock(slim::queue::queue_mutex);
		job->egress_job_file.file_name_string = utilities::v8ValueToString(isolate, v8_file_name_string);
		job->egress_job_file.storage_container_handle = utilities::v8ValueToString(isolate, v8_storage_container_handle);
		job->completed = true;
	}
	else {
		log::debug(log::Message("slim::plugin::queue::job::complete()",std::string("job not found => " + queue_name_string).c_str(),__FILE__,__LINE__));
	}
	log::trace(log::Message("slim::plugin::queue::job::complete()", "ends",__FILE__, __LINE__));
}
void slim::plugin::queue::listen(const v8::FunctionCallbackInfo<v8::Value>& args) {
	auto* isolate = args.GetIsolate();
	auto queue_name_string = utilities::v8ValueToString(isolate, args[0]);
	log::trace(log::Message("slim::plugin::queue::listen()", "begins, queue name => " + queue_name_string,__FILE__, __LINE__));
	v8::HandleScope handle_scope(isolate);
	auto context = isolate->GetCurrentContext();
	auto maybe_resolver = v8::Promise::Resolver::New(context);
	if(maybe_resolver.IsEmpty()) {
		isolate->ThrowException(utilities::StringToV8String(isolate, "slim::plugin::queue::listen() failed to create promise"));
	}
	auto resolver = maybe_resolver.ToLocalChecked();
	args.GetReturnValue().Set(resolver->GetPromise());
	
	auto job_future = std::async(std::launch::async, slim::queue::get_job, queue_name_string);
	auto* job = job_future.get();
	auto v8_job_object = v8::Object::New(isolate);
	auto v8_ingress_file_store_object = v8::Object::New(isolate);
	auto v8_egress_file_store_object = v8::Object::New(isolate);
	auto result_of_set = v8_egress_file_store_object->Set(context, utilities::StringToV8String(isolate, "file_name_string"), utilities::StringToV8String(isolate, ""));
	result_of_set = v8_egress_file_store_object->Set(context, utilities::StringToV8String(isolate, "storage_container_handle"), utilities::StringToV8String(isolate, job->egress_job_file.storage_container_handle));
	result_of_set = v8_job_object->Set(context, utilities::StringToV8String(isolate, "egress_job_file"), v8_egress_file_store_object);
	result_of_set = v8_ingress_file_store_object->Set(context, utilities::StringToV8String(isolate, "file_name_string"), utilities::StringToV8String(isolate, job->ingress_job_file.file_name_string));
	result_of_set = v8_ingress_file_store_object->Set(context, utilities::StringToV8String(isolate, "storage_container_handle"), utilities::StringToV8String(isolate, job->ingress_job_file.storage_container_handle));
	result_of_set = v8_job_object->Set(context, utilities::StringToV8String(isolate, "ingress_job_file"), v8_ingress_file_store_object);
	result_of_set = v8_job_object->Set(context, utilities::StringToV8String(isolate, "errors"), v8::Array::New(isolate));
	result_of_set = v8_job_object->Set(context, utilities::StringToV8String(isolate, "queue_name_string"), utilities::StringToV8String(isolate, job->queue_name_string));
	//v8_job_object->Set(context, utilities::StringToV8String(isolate, "errored"), v8::Boolean::New(isolate, true));
	result_of_set = v8_job_object->Set(context, utilities::StringToV8String(isolate, "ticket_id"), v8::Integer::New(isolate, job->get_ticket_id()));
	auto job_complete_function_template = v8::FunctionTemplate::New(isolate, job::complete);
	auto job_complete_function = job_complete_function_template->GetFunction(context).ToLocalChecked();
	result_of_set = v8_job_object->Set(context, utilities::StringToV8String(isolate, "complete"), job_complete_function);
	auto result_of_resolve = resolver->Resolve(context, v8_job_object);
	log::trace(log::Message("slim::plugin::queue::listen()", "ends with job received from , queue name => " + queue_name_string + ", job name => " + job->ingress_job_file.file_name_string ,__FILE__, __LINE__));
}
extern "C" void expose_plugin(v8::Isolate* isolate) {
	slim::plugin::plugin queue_plugin(isolate, "queue");
	queue_plugin.add_function("listen", slim::plugin::queue::listen);
	queue_plugin.expose_plugin();
	return;
}
