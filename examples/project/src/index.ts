import console from 'console';
import server from 'http';
import helloWorld from './hello_world.ts';

console.trace("begin");

// Wrap your server logic in a reusable async function
async function startServer(name, port) {
    const config = { host: '0.0.0.0', port: port };
    console.debug(`[${name}] starting on port ${port}`, config);

    const listener = server.serve(config);
    console.debug(`[${name}] listener created`);

    let requestCount = 0;

    // This loop runs independently for each server instance
    for await (const event of listener) {
        console.debug(`[${name}] event found`);
        const response = event.response;
        const timestamp = new Date().toISOString();
        const id = ++requestCount;

        await response.reply(
          `<html><body><h1>${helloWorld} from ${name}!</h1><p>request: ${id}</p><p>src: ts</p><p>time: ${timestamp}</p></body></html>`,
            { status: 200, statusText: 'OK' }
        );

        console.debug(`[${name}:${requestCount}] response sent`);

        // if (requestCount === 1000) {
        //     console.debug(`[${name}] stopping server`);
        //     listener.stop();
        // }
    }
}

// Call the function twice without 'await' to start them concurrently
startServer('Server 1', 9000);
//startServer('Server 2', 9001);
