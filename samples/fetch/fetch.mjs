import console from 'console'
import process from 'process'
slim.load("http_package");
const url = process.argv[2];
console.info("fetching file =>", url);
const response = await fetch(url);
console.debug(response.data);
