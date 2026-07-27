declare interface SlimHttpServerConfig {
    host: string;
    port: number;
    cert?: string;
    key?: string;
}

declare interface SlimHttpEvent {
    request: {
        url: string;
    };
}

declare interface SlimHttpListener {
    next(): Promise<{ value: SlimHttpEvent; done: boolean }>;
    [Symbol.asyncIterator](): SlimHttpListener;
}

declare module 'http' {
    const server: {
        serve(config: SlimHttpServerConfig): SlimHttpListener;
    };
    export default server;
}
