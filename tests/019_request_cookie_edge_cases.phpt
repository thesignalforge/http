--TEST--
signalforge_http: Request PSR-7 ServerRequestInterface cookie edge cases
--EXTENSIONS--
signalforge_http
--FILE--
<?php
use Signalforge\NativeHttp\Request;

// ARRANGE: Set up basic request
$_SERVER = [
    'REQUEST_METHOD' => 'GET',
    'REQUEST_URI' => '/test',
];
$_GET = [];
$_POST = [];
$_COOKIE = [];
$_FILES = [];

// ACT: Capture request
$request = Request::capture();

// ASSERT: Empty cookies by default
var_dump($request->getCookieParams() === []);

// ACT: Test withCookieParams with various scenarios
$cookieRequest1 = $request->withCookieParams(['session' => 'abc123', 'theme' => 'dark']);
var_dump($cookieRequest1->getCookieParams() === ['session' => 'abc123', 'theme' => 'dark']);

// ACT: Test withCookieParams with empty array
$emptyCookieRequest = $request->withCookieParams([]);
var_dump($emptyCookieRequest->getCookieParams() === []);

// ACT: Test withCookieParams with malformed cookies
$malformedCookies = [
    ['session' => ['nested' => 'value']], // Nested array should be rejected
    null,
    'string',
    123,
];

foreach ($malformedCookies as $cookies) {
    try {
        $request->withCookieParams($cookies);
        var_dump(false); // Should not reach here
    } catch (InvalidArgumentException $e) {
        var_dump(true); // Exception correctly thrown
    } catch (Exception $e) {
        var_dump(false); // Wrong exception type
    }
}

// ACT: Test withCookieParams with special characters in values
$specialCookieRequest = $request->withCookieParams([
    'encoded' => 'value%20with%20spaces',
    'json' => '{"key": "value"}',
    'special' => 'chars:!@#$%^&*()',
]);
var_dump($specialCookieRequest->getCookieParams()['encoded'] === 'value%20with%20spaces');
var_dump($specialCookieRequest->getCookieParams()['json'] === '{"key": "value"}');
var_dump($specialCookieRequest->getCookieParams()['special'] === 'chars:!@#$%^&*()');

// ACT: Test withCookieParams with very large cookie values
$largeValue = str_repeat('x', 4096); // 4KB cookie
$largeCookieRequest = $request->withCookieParams(['large' => $largeValue]);
var_dump(strlen($largeCookieRequest->getCookieParams()['large']) === 4096);

// ACT: Test withCookieParams with numeric keys (converted to strings)
$numericKeyRequest = $request->withCookieParams([123 => 'value', '456' => 'other']);
$cookies = $numericKeyRequest->getCookieParams();
var_dump(isset($cookies['123']));
var_dump(isset($cookies['456']));

// ASSERT: Immutability maintained
var_dump($request->getCookieParams() === []);
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
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
