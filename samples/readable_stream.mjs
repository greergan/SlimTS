import console from 'console'
console.log('here')
let r = new ReadableStream();
console.log(r)
let textFile = "abc";
let f = new TextDecoder().decode(textFile);
console.log(f)
let rr = new Response();
console.log(rr);