	v8::HandleScope handle_scope(_isolate);
	auto context = _isolate->GetCurrentContext();
	auto template = v8::ObjectTemplate::New(_isolate);
	template->SetInternalFieldCount(_count);
	template->NewInstance(context).ToLocal(&_object);