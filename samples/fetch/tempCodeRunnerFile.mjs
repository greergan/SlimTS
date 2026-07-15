
for (const target of to_fetch) {
	console.info(__line_number, "fetching => ", target);
	const response = await fetch();
	console.debug(response);
	console.info(__line_number, "fetched => ", target);
}
