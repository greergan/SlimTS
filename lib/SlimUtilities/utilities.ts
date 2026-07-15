import { is_file_url, is_valid_url } from "./validations.ts";
export function get_content_type(uri:string):string|undefined {
    console.debug({message:"starting with", value:"uri"}, uri);
    let content_type:string|undefined = undefined;
    const file_extenssion:string = uri.substring(uri.lastIndexOf('.') +1);
    console.debug({message:"file extenssion", value:file_extenssion});
    if(['htm','html'].includes(file_extenssion) || uri.endsWith('/')) { content_type = "text/html; charset=utf-8"; }
    else if(uri.endsWith('.css'))  { content_type = "text/css; charset=utf-8"; }
    else if(uri.endsWith('.js'))   { content_type = "text/javascript; charset=utf-8"; }
    else if(uri.endsWith('.json')) { content_type = "text/json; charset=utf-8"; }
    else if(uri.endsWith('.ico'))  { content_type = "image/x-icon"; }
    else if(uri.endsWith('.svg'))  { content_type = "image/svg"; }
    else if(uri.endsWith('.apng')) { content_type = "image/apng"; }
    else if(uri.endsWith('.png'))  { content_type = "image/png"; }
    else if(['jpg','jpeg','jpe','jif','jfif'].includes(file_extenssion)) { content_type = "image/jpeg"; }
    else if(uri.endsWith('.gif'))  { content_type = "image/gif"; }
    else if(uri.endsWith('.avif')) { content_type = "image/avif"; }
    else if(uri.endsWith('.webp')) { content_type = "image/webp"; }
    else if(uri.endsWith('.csv'))  { content_type = "text/csv; charset=utf-8"; }
    else if(uri.endsWith('.rtf'))  { content_type = "text/rtf; charset=utf-8"; }
    else if(uri.endsWith('.xml'))  { content_type = "text/xml; charset=utf-8"; }
    else if(['txt','text'].includes(file_extenssion)) { content_type = "text/plain; charset=utf-8"; }
    console.trace(content_type);
    return content_type;
}
export async function get_binary_contents(file:string): Promise<any|undefined> {
        const blob:Blob = await (await fetch(file)).blob();
        if(window.hasOwnProperty('SlimConsole') && blob) console.trace({message:"fetch",value:blob ? "succeded" : "failed"}, file);
        console.trace();
        return blob;
}
export async function get_file_contents(file:string): Promise<string|undefined> {
        const text:string|undefined = await (await fetch(file)).text();
        if(window.hasOwnProperty('SlimConsole') && text) console.trace({message:"fetch",value:text ? "succeded" : "failed"}, file);
        console.trace();
        return text;
}
export async function get_json_contents(file:string): Promise<slim.types.iKeyValueAny|undefined> {
    console.debug({message:"starting with", value:"file"}, file, "time to add error handling from fetch results");
    const json:slim.types.iKeyValueAny|undefined = await (await fetch(file)).json();
    if(window.hasOwnProperty('SlimConsole') && json) console.trace({message:"fetch",value:json ? "succeded" : "failed"}, file);
    console.trace();
    return json;
}
export function get_normalized_uri(uri:string):string {
    const normalized_uri:string = uri && uri.length > 1 && uri.endsWith('/') ? uri.substring(0, uri.lastIndexOf('/')) : uri;
    console.trace("get_normalized_uri()", normalized_uri);
    return normalized_uri;
}
export async function get_normalized_url(property:string): Promise<string|undefined> {
    const cwd = Deno.cwd();
    let normalized_url:string|undefined = undefined;
    if(property.startsWith("./")) {
        const new_value = property.replace("./", "");
        normalized_url = (new_value.length > 1) ? `file://${cwd}/${new_value}` : `file://${cwd}`;
    }
    else if(property == ".") {
        normalized_url = `file://${cwd}`;
    }
    else if(property.startsWith("..")) {
        let new_value = property;
        let new_cwd = cwd;
        while(new_value.startsWith("..")) {
            if(new_value.startsWith("../")) {
                new_value = new_value.substring(3);
            }
            else if(new_value.startsWith("..")) {
                new_value = new_value.substring(2);
            }
            new_cwd = new_cwd.substring(0, new_cwd.lastIndexOf("/"));
        }
        normalized_url = (new_value.length == 0) ? `file://${new_cwd}` : `file://${new_cwd}/${new_value}`;
    }
    else if(is_valid_url(property)) {
        normalized_url = property;
    }
    else {
        //normalized_url = `file://${cwd}/${property}`;
        normalized_url = `file://${property}`;
    }
    if(normalized_url.endsWith("/")) {
        normalized_url = normalized_url.substring(0, normalized_url.lastIndexOf("/"));
    }
    if(window.hasOwnProperty('SlimConsole')) console.trace(normalized_url);
    return normalized_url;
}
export async function write_output(output_file:string, content:string) {
    const file = get_absolute_file_path(output_file);
    if(file !== undefined && file.length > 1) {
        try {
            Deno.mkdirSync(file.substring(0, file.lastIndexOf("/")), { recursive: true });
            Deno.writeTextFileSync(file, content);
        }
        catch(e) {
            if(window.hasOwnProperty('SlimConsole')) SlimConsole.abort({message:"write_output"}, e);
        }
    }
    else {
        if(window.hasOwnProperty('SlimConsole')) SlimConsole.abort({message: "not a valid file url"}, output_file);
    }
    if(window.hasOwnProperty('SlimConsole')) console.trace();
}