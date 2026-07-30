declare interface SlimHttpServerConfig {
    host: string;
    port: number;
    cert?: string;
    key?: string;
}

declare interface SlimHttpHeaders {
    get(name: string): string | null;
    set(name: string, value: string): void;
    has(name: string): boolean;
    delete(name: string): void;
    forEach(callback: (value: string, key: string) => void): void;
    entries(): IterableIterator<[string, string]>;
    keys(): IterableIterator<string>;
    values(): IterableIterator<string>;
}

declare interface SlimHttpRequest {
    url: string;
    method: string;
    headers: SlimHttpHeaders;
    bodyUsed: boolean;
    credentials: string;
    cache: string;
    redirect: string;
    referrer: string;
    referrerPolicy: string;
    mode: string;
    integrity: string;
    keepalive: boolean;
    signal: { aborted: boolean };
    text(): Promise<string>;
    json(): Promise<any>;
    arrayBuffer(): Promise<ArrayBuffer>;
    clone(): SlimHttpRequest;
}

declare interface SlimHttpResponse {
    status: number;
    statusText: string;
    ok: boolean;
    redirected: boolean;
    type: string;
    url: string;
    bodyUsed: boolean;
    headers: SlimHttpHeaders;
    reply(body: string, init?: { status?: number; statusText?: string }): Promise<void>;
    text(): Promise<string>;
    json(): Promise<any>;
    arrayBuffer(): Promise<ArrayBuffer>;
    clone(): SlimHttpResponse;
    error(): { status: number; type: string; ok: boolean };
}

declare interface SlimHttpEvent {
    request: SlimHttpRequest;
    response: SlimHttpResponse;
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
