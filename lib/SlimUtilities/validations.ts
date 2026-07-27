import fs from 'fs';
// '[^]A-Za-z0-9_.~!*''();:@&=+$,/?#[%-]+' are all valid in URI strings
export async function is_binary_content_type(content_type_string:string|undefined):Promise<boolean|undefined> {
    if(content_type_string === undefined) {
        return undefined;
    }
    return content_type_string.toLowerCase().startsWith("image/") ? true : false;
}
export async function is_directory(url_string:string|undefined):Promise<boolean|undefined> {
    if(url_string === undefined) {
        return undefined;
    }
/*     let isDirectory:boolean = false;
    if(await is_file_url(url_string)) {
        const results = await get_absolute_file_path(url_string);
        if(results !== undefined) {
            isDirectory = (await Deno.stat(results!)).isDirectory;
        }
    }
    if(window.hasOwnProperty('SlimConsole')) console.trace(isDirectory);
    return isDirectory; */
}
export async function is_file(url_string:string|undefined):Promise<boolean|undefined> {
    if(url_string === undefined) {
        return undefined;
    }
/*     let isFile:boolean = false;
    if(url !== undefined && await is_file_url(url_string)) {
        const results = await get_absolute_file_path(url_string);
        if(results !== 'undefined') {
            isFile = (await Deno.stat(results!)).isFile;
        }
    }
    if(window.hasOwnProperty('SlimConsole')) console.trace(isFile);
    return isFile; */
}
export async function is_file_url(url_string:string|undefined):Promise<boolean|undefined> {
    if(url_string === undefined) {
        return undefined;
    }
    return url_string.toLocaleLowerCase().startsWith("file://") ? true: false;
}
export async function is_http_url(url_string:string|undefined):Promise<boolean|undefined> {
    if(url_string === undefined) {
        return undefined;
    }
    return url_string.toLocaleLowerCase().startsWith("http://") ? true: false;
}
export async function is_https_url(url_string:string|undefined):Promise<boolean|undefined> {
    if(url_string === undefined) {
        return undefined;
    }
    return url_string.toLocaleLowerCase().startsWith("https://") ? true: false;
}
export async function is_text_content_type(content_type_string:string|undefined):Promise<boolean|undefined> {
    if(content_type_string === undefined) {
        return undefined;
    }
    return content_type_string.toLowerCase().startsWith("text/") ? true : false;
}
export async function is_valid_url(url_string:string|undefined):Promise<boolean|undefined> {
    if(url_string === undefined) {
        return undefined;
    }
    return (await is_file_url(url_string) || await is_http_url(url_string) || await is_https_url(url_string));
}