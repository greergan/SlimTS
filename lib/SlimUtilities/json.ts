import * as slim from "./slim_modules.ts";
export function array_contains(array:slim.types.iKeyValueAny[], check_for_member:object, merge_arrays:boolean=false):boolean {
	let is_in:boolean = false;
	const b_keys:string[] = (Object.keys(check_for_member)).sort();
	const b_keys_string:string = JSON.stringify(b_keys);
	let keys_checked:number = 0;
	for(const member of array) {
		const a_keys:string[] = (Object.keys(member)).sort();
		if(JSON.stringify(a_keys) !== b_keys_string) {
			is_in = false;
			break;
		}
		let keys_matched:number = 0;
		for(const key of Object.keys(member)) {
			keys_checked++;
			if(['string','boolean','number'].includes(typeof member[key])) {
				if(member[key] == check_for_member[key]) {
					keys_matched++;
				}
			}
			else if(Array.isArray(member[key]) && Array.isArray(check_for_member[key])) {
				if(member[key].length == check_for_member[key].length && member[key].length == 0) {
					keys_matched++;
				}
				else {
					let members_match:boolean = false;
					let member_matches:number = 0;
					member[key].forEach((array_member:any) => {
						
						if(['string','number','boolean'].includes(typeof array_member)) {
							if(check_for_member[key].includes(array_member)) {
								member_matches++;
							}
						}
					});
					check_for_member[key].forEach((array_member:any) => {
						if(member[key].includes(array_member)) {
							member_matches++;
						}
					});
					if(member_matches = (member[key].length + check_for_member[key])) {
						members_match = true;
					}
/* 					if(!members_match && merge_arrays) {
						check_for_member[key].forEach((array_member:any) => {
							member[key].push(array_member)
						});
						keys_matched++;
					} */
				}
			}
		}
		if(keys_matched == a_keys.length) {
			is_in = true;
			break;
		}
	}
	return is_in;
}
declare interface comingle_options {
	skip?:string[],
	excludes?:string[]
	depth?:number
}
export function copy_ofSync(source:slim.types.iKeyValueAny, options?:comingle_options) : slim.types.iKeyValueAny {
	return comingleSync([{}, source], options);
}
declare interface localized_comingle_options {
	skip:string[],
	excludes:string[],
	depth:number
}
export function comingleSync(input_sources:slim.types.iKeyValueAny[], options?:comingle_options): slim.types.iKeyValueAny {
	const localized_options:localized_comingle_options = (options) ? JSON.parse(JSON.stringify(options)) : {depth:1};
	if(!localized_options.hasOwnProperty('depth')) localized_options.depth = 1;
	['skip','excludes'].map(element => { if(!localized_options[element]) localized_options[element] = []});
	if(input_sources.length < 2) {
		throw new Error("comingle requires an array of 2 or more slim.types.iKeyValueAny objects");
	}
	const sources:slim.types.iKeyValueAny[] = JSON.parse(JSON.stringify(input_sources));
	const merged_objects:slim.types.iKeyValueAny = {};
	for(const source of sources) {
		for(const key in source) {
			const key_type:string = typeof source[key];
			if(['string','number','boolean'].includes(key_type)) {
				let continue_primitive_processing = true;
				if(localized_options['skip'].includes(key)) {
					continue_primitive_processing = false;
				}
				if(localized_options['excludes'].includes(key_type)) {
					continue_primitive_processing = false;
				}
				if(continue_primitive_processing) {
					merged_objects[key] = source[key];
				}
			}
			else if(Array.isArray(source[key])) {
				let continue_array_processing:boolean = true;
				if(localized_options['skip'].includes(key)) {
					continue_array_processing = false;
				}
				if(localized_options['excludes']!.includes('array')) {
					continue_array_processing = false;
				}
				if(continue_array_processing) {
					if(typeof merged_objects[key] == 'undefined') {
						merged_objects[key] = [];
						merged_objects[key] = source[key];
						continue_array_processing = false;
					}
				}
				if(continue_array_processing) {
 					for(const member of source[key]) {
						if(!this.array_contains(merged_objects[key], member)) {
							merged_objects[key].push(member);
						}

/* 						let lvalue_exists:boolean = false;
						let keys_matched:number = 0;
						for(const index in merged_objects[key]) {
							Object.keys(merged_objects[key][index]).forEach(k => {
								if(typeof merged_objects[key][index][k] != 'object') {
									lvalue_exists = (merged_objects[key][index][k] == member[k]);
									if(found_gahan && lvalue_exists) {
										console.log(merged_objects[key][index][k], member[k])
										console.log(sources[0][key][index].first_name, member.first_name);
									}
								}
							});
						}
						if(!lvalue_exists) {
							merged_objects[key].push(member);
						} */
					}
				}
			}
			else if(key_type === 'object') {
				let continue_object_processing = true;
				if(localized_options['skip'].includes(key)) {
					continue_object_processing = false;
				}
				if(localized_options['excludes'].includes(key_type)) {
					continue_object_processing = false;
				}
				if(continue_object_processing) {
					if(typeof merged_objects[key] == 'undefined') {
						merged_objects[key] = {};
					}
					merged_objects[key] = comingleSync([merged_objects[key], source[key]], {depth: localized_options.depth++});
				}
			}
		}
	}
	return merged_objects as slim.types.iKeyValueAny;
}
export async function get_node_value(model:slim.types.iKeyValueAny, property:string): Promise<string|number|boolean|slim.types.iKeyValueAny|undefined> {
	console.debug({message:"beginning with", value:"property"}, property, model);
	if(model === undefined || Object.keys(model).length == 0) {
		return undefined;
	}
	let shifted_model:slim.types.iKeyValueAny = model;
	property.trim().split('.').forEach((node_name:string) => {
		const model_has_index_match:string[] = node_name.match(/([\w\d]+)\[(\d)\]$/) ?? [];
		shifted_model = model_has_index_match.length == 3 
			? shifted_model[model_has_index_match[1]] && [model_has_index_match[2]] 
				? shifted_model[model_has_index_match[1]][model_has_index_match[2]] 
					: undefined : shifted_model && shifted_model[node_name] ? shifted_model[node_name] : undefined;
	});
	return shifted_model;
}
export function get_value<Type, Key extends keyof Type>(obj: Type, key: Key) { return obj[key]; }