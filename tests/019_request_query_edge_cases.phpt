--TEST--
signalforge_http: Request PSR-7 ServerRequestInterface query parameter edge cases
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

// ASSERT: Empty query params by default
var_dump($request->getQueryParams() === []);

// ACT: Test withQueryParams with various scenarios
$queryRequest1 = $request->withQueryParams(['page' => '1', 'sort' => 'name']);
var_dump($queryRequest1->getQueryParams() === ['page' => '1', 'sort' => 'name']);

// ACT: Test withQueryParams with empty array
$emptyQueryRequest = $request->withQueryParams([]);
var_dump($emptyQueryRequest->getQueryParams() === []);

// ACT: Test withQueryParams with malformed query params
$malformedQueries = [
    ['page' => ['nested']], // Nested arrays
    null,
    'string',
    123,
];

foreach ($malformedQueries as $query) {
    try {
        $request->withQueryParams($query);
        var_dump(false); // Should not reach here
    } catch (InvalidArgumentException $e) {
        var_dump(true); // Exception correctly thrown
    } catch (Exception $e) {
        var_dump(false); // Wrong exception type
    }
}

// ACT: Test withQueryParams with URL-encoded values
$urlEncodedRequest = $request->withQueryParams([
    'search' => 'hello%20world',
    'filter' => 'name%3Djohn%26age%3D25',
    'special' => 'chars:!@#$%^&*()',
]);
var_dump($urlEncodedRequest->getQueryParams()['search'] === 'hello%20world');
var_dump($urlEncodedRequest->getQueryParams()['filter'] === 'name%3Djohn%26age%3D25');
var_dump($urlEncodedRequest->getQueryParams()['special'] === 'chars:!@#$%^&*()');

// ACT: Test withQueryParams with numeric values
$numericQueryRequest = $request->withQueryParams([
    'page' => 1,
    'limit' => 10,
    'active' => true,
    'inactive' => false,
]);
$queryParams = $numericQueryRequest->getQueryParams();
var_dump($queryParams['page'] === 1);
var_dump($queryParams['limit'] === 10);
var_dump($queryParams['active'] === true);
var_dump($queryParams['inactive'] === false);

// ACT: Test withQueryParams with very large parameter values
$largeValue = str_repeat('x', 8192); // 8KB parameter
$largeQueryRequest = $request->withQueryParams(['data' => $largeValue]);
var_dump(strlen($largeQueryRequest->getQueryParams()['data']) === 8192);

// ACT: Test withQueryParams with empty string values
$emptyStringRequest = $request->withQueryParams(['empty' => '', 'null' => null]);
$queryParams = $emptyStringRequest->getQueryParams();
var_dump($queryParams['empty'] === '');
var_dump($queryParams['null'] === null);

// ASSERT: Immutability maintained
var_dump($request->getQueryParams() === []);
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
bool(true)
bool(true)
bool(true)
bool(true)
