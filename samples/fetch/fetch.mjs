import console from 'console';
slim.load("http_package");

const to_fetch = ["", "abcdef", "a b c d e f:", "abcdef:", "file:// \t\n\r\",'<>`{}|\'^[];"];

for (const target of to_fetch) {
	console.info(__line_number, `fetching => "${target}"`);
	const response = await fetch(target);
	console.debug(response);
	console.info(__line_number, `fetching => "${target}"`);
}
