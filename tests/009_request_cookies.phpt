--TEST--
signalforge_http: Request PSR-7 ServerRequestInterface getCookieParams() and withCookieParams()
--EXTENSIONS--
signalforge_http
--FILE--
<?php
use Signalforge\NativeHttp\Request;

// ARRANGE: Set up request with cookies
$_SERVER = [
    'REQUEST_METHOD' => 'GET',
    'REQUEST_URI' => '/test',
];
$_GET = [];
$_POST = [];
$_COOKIE = [
    'session_id' => 'abc123',
    'user_pref' => 'dark_mode',
    'lang' => 'en',
];
$_FILES = [];

// ACT: Capture request
$request = Request::capture();

// ASSERT: Cookies retrieved
$cookies = $request->getCookieParams();
var_dump($cookies['session_id'] === 'abc123');
var_dump($cookies['user_pref'] === 'dark_mode');
var_dump($cookies['lang'] === 'en');
var_dump(is_array($cookies));

// ACT: Replace cookies
$newCookies = ['new_session' => 'xyz789'];
$newRequest = $request->withCookieParams($newCookies);

// ASSERT: Cookies replaced, original unchanged
var_dump($newRequest->getCookieParams() === $newCookies);
var_dump($request->getCookieParams() !== $newCookies);
var_dump($request !== $newRequest);
var_dump($request->getCookieParams()['session_id'] === 'abc123'); // Original unchanged

// ARRANGE: Test empty cookies
$_COOKIE = [];
$noCookieRequest = Request::capture();

// ASSERT: Empty cookies
var_dump($noCookieRequest->getCookieParams() === []);
?>
--CLEAN--
<?php
$_SERVER = [];
$_GET = [];
$_POST = [];
$_COOKIE = [];
$_FILES = [];
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)

