const char MAIN_page[] PROGMEM = R"=====(
<HTML>
	<HEAD>
		<TITLE>LEDControl</TITLE>
		<style>
			body {
				font-family: Arial, sans-serif;
				text-align: center;
				margin: 50px;
			}
			button, a {
				margin: 10px;
				padding: 10px 20px;
				font-size: 18px;
				cursor: pointer;
				display: inline-block;
				background-color: #007BFF;
				color: white;
				text-decoration: none;
			}
		</style>
	</HEAD>
<BODY>
	<CENTER>
		<H1>WeatherLED Control Website</H1>
		<p>Click a button to switch to the mentioned Mode</p>

		<!-- Links that change the URL -->
		<a href="/snow">snow</a>
		<a href="/rain">rain</a>
		<a href="/clear">clear</a>
		<a href="/scraper">scraper</a>
		<a href="/off">off</a>

	</CENTER>	
</BODY>
</HTML>
)=====";