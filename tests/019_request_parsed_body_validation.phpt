--TEST--
signalforge_http: Request PSR-7 ServerRequestInterface parsed body validation
--EXTENSIONS--
signalforge_http
--FILE--
<?php
use Signalforge\NativeHttp\Request;

// ARRANGE: Set up basic request
$_SERVER = [
    'REQUEST_METHOD' => 'POST',
    'REQUEST_URI' => '/test',
    'HTTP_CONTENT_TYPE' => 'application/json',
];
$_GET = [];
$_POST = [];
$_COOKIE = [];
$_FILES = [];

// ACT: Capture request
$request = Request::capture();

// ASSERT: Default parsed body is null
var_dump($request->getParsedBody() === null);

// ACT: Test withParsedBody with valid array
$arrayBodyRequest = $request->withParsedBody(['key' => 'value', 'nested' => ['data' => 123]]);
var_dump($arrayBodyRequest->getParsedBody() === ['key' => 'value', 'nested' => ['data' => 123]]);

// ACT: Test withParsedBody with valid object
$object = new stdClass();
$object->property = 'value';
$objectBodyRequest = $request->withParsedBody($object);
var_dump($objectBodyRequest->getParsedBody() === $object);

// ACT: Test withParsedBody with null (valid)
$nullBodyRequest = $request->withParsedBody(null);
var_dump($nullBodyRequest->getParsedBody() === null);

// ACT: Test withParsedBody with invalid types
$invalidTypes = [
    'string', // String
    123, // Integer
    12.34, // Float
    true, // Boolean
    false, // Boolean
    fopen('php://memory', 'r+'), // Resource
];

foreach ($invalidTypes as $invalidBody) {
    try {
        $request->withParsedBody($invalidBody);
        var_dump(false); // Should not reach here
    } catch (InvalidArgumentException $e) {
        var_dump(true); // Exception correctly thrown
    } catch (Exception $e) {
        var_dump(false); // Wrong exception type
    }
}

// ACT: Test withParsedBody with deeply nested structures
$deepNestedRequest = $request->withParsedBody([
    'level1' => [
        'level2' => [
            'level3' => [
                'data' => 'deep',
                'array' => [1, 2, 3],
                'object' => (object)['key' => 'value']
            ]
        ]
    ]
]);
$parsedBody = $deepNestedRequest->getParsedBody();
var_dump($parsedBody['level1']['level2']['level3']['data'] === 'deep');
var_dump($parsedBody['level1']['level2']['level3']['array'] === [1, 2, 3]);
var_dump($parsedBody['level1']['level2']['level3']['object']->key === 'value');

// ACT: Test withParsedBody with empty arrays/objects
$emptyArrayRequest = $request->withParsedBody([]);
var_dump($emptyArrayRequest->getParsedBody() === []);

$emptyObject = new stdClass();
$emptyObjectRequest = $request->withParsedBody($emptyObject);
var_dump($emptyObjectRequest->getParsedBody() === $emptyObject);

// ACT: Test getParsedBody with $_POST data when no explicit parsed body set
$_SERVER['REQUEST_METHOD'] = 'POST';
$_SERVER['HTTP_CONTENT_TYPE'] = 'application/x-www-form-urlencoded'; // POST data should have form content type
$_POST = ['form_field' => 'form_value', 'another' => '123'];
$postRequest = Request::capture();
$parsedPost = $postRequest->getParsedBody();
var_dump($parsedPost['form_field'] === 'form_value');
var_dump($parsedPost['another'] === '123');

// ACT: Test getParsedBody returns null when $_POST is empty
$_POST = [];
$emptyPostRequest = Request::capture();
var_dump($emptyPostRequest->getParsedBody() === null);

// ASSERT: Immutability maintained
var_dump($request->getParsedBody() === null);
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
bool(true)
