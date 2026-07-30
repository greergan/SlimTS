import console from 'console';
import server from 'http';
import helloWorld from '/hello_world';
console.trace("begin");
const config: SlimHttpServerConfig = { host: '0.0.0.0', port: 9000 };
async function serve(config: SlimHttpServerConfig) {
    console.debug(config);
    const listener = server.serve(config);
    console.debug("listener created");
    let requestCount = 0;
    for await (const event of listener) {
        console.debug("event found");
        //const request = event.request;
        // console.debug(event.request);
        // console.debug(`url: ${request.url}`);
        // console.debug(typeof request.headers);
        // request.headers.forEach((value, key) => {
        //     console.debug(`forEach: ${key} ==> ${value}`);
        // });
        // for (const pair of request.headers.entries()) {
        //     console.debug(`entries: ${pair[0]}: ${pair[1]}`);
        // }
        // for (const value of request.headers.values()) {
        //     console.debug(`values: ${value}`);
        // }
        // for (const key of request.headers.keys()) {
        //     console.debug(`keys: ${key}`);
        // }
        const response = event.response;
        const timestamp = new Date().toISOString();
        const id = ++requestCount;
        await response.reply(`<html><body><h1>${helloWorld}</h1><p>request: ${id}</p><p>time: ${timestamp}</p></body></html>`, { status: 200, statusText: 'OK' });
        console.debug("response sent");
    }
}
serve(config);
