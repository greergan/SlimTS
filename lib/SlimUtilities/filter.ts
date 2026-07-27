import * as slim from "./slim_modules.ts";
import * as filter from "http://192.168.122.59/slim.filter/index.ts";
export async function filter(model:Array<slim.types.iKeyValueAny>, filter:filter.SlimFilter):Promise<Array<slim.types.iKeyValueAny>> {
    console.debug({message:"beginning with", value:"model"}, model.length);
    console.debug({message:"beginning with", value:"filter.predicate"}, filter.predicate);
    console.debug({message:"beginning with", value:"filter.property"}, filter.property);
    console.debug({message:"beginning with", value:"filter.value"}, filter.value);
    let new_model:slim.types.iKeyValueAny = [];
    switch(filter.predicate.toLowerCase()) {
        case 'equal':
            new_model = model.filter((member:slim.types.iKeyValueAny, index:number) => {
                let value:string|number|boolean = "";
                switch(typeof member[filter.property]) {
                    case 'boolean':
                    case 'number':
                        value = JSON.parse(filter.value.toLowerCase());
                        break;
                    case 'string':
                        value = filter.value;
                        break;
                }
                if(member[filter.property] == value) {
                    filter.new_model_matched_indexes.push(index);
                    return member;
                }
            });
            break;
        case 'not_equal':
            new_model = model.filter((member:slim.types.iKeyValueAny, index:number) => {
                let value:string|number|boolean = "";
                switch(typeof member[filter.property]) {
                    case 'boolean':
                    case 'number':
                        value = JSON.parse(filter.value.toLowerCase());
                        break;
                    case 'string':
                        value = filter.value;
                        break;
                }
                if(member[filter.property] != value) {
                    filter.new_model_matched_indexes.push(index);
                    return member;
                }
            });
            break;
        case 'greater_than':
            new_model = model.filter((member:slim.types.iKeyValueAny, index:number) => {
                let value:string|number|boolean = "";
                switch(typeof member[filter.property]) {
                    case 'boolean':
                    case 'number':
                        value = JSON.parse(filter.value.toLowerCase());
                        break;
                    case 'string':
                        value = filter.value;
                        break;
                }
                if(member[filter.property] > value) {
                    filter.new_model_matched_indexes.push(index);
                    return member;
                }
            });
            break;
        case 'greater_than_equal':
            new_model = model.filter((member:slim.types.iKeyValueAny, index:number) => {
                let value:string|number|boolean = "";
                switch(typeof member[filter.property]) {
                    case 'boolean':
                    case 'number':
                        value = JSON.parse(filter.value.toLowerCase());
                        break;
                    case 'string':
                        value = filter.value;
                        break;
                }
                if(member[filter.property] >= value) {
                    filter.new_model_matched_indexes.push(index);
                    return member;
                }
            });
            break;
        case 'less_than':
            new_model = model.filter((member:slim.types.iKeyValueAny, index:number) => {
                let value:string|number|boolean = "";
                switch(typeof member[filter.property]) {
                    case 'boolean':
                    case 'number':
                        value = JSON.parse(filter.value.toLowerCase());
                        break;
                    case 'string':
                        value = filter.value;
                        break;
                }
                if(member[filter.property] < value) {
                    filter.new_model_matched_indexes.push(index);
                    return member;
                }
            });
            break;
        case 'less_than_equal':
            new_model = model.filter((member:slim.types.iKeyValueAny, index:number) => {
                let value:string|number|boolean = "";
                switch(typeof member[filter.property]) {
                    case 'boolean':
                    case 'number':
                        value = JSON.parse(filter.value.toLowerCase());
                        break;
                    case 'string':
                        value = filter.value;
                        break;
                }
                if(member[filter.property] <= value) {
                    filter.new_model_matched_indexes.push(index);
                    return member;
                }
            });
            break;

        default:
            console.warn({message:"filter.handler", value:"not supported"}, filter.handler);
            break;
    }
    console.trace({message:"filter",value:"returning model.length"}, new_model.length);
    return new_model;
}