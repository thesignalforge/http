--TEST--
signalforge_http: UploadedFile PSR-17 create() factory method
--EXTENSIONS--
signalforge_http
--FILE--
<?php
use Signalforge\NativeHttp\UploadedFile;
use Signalforge\NativeHttp\Stream;
use Psr\Http\Message\UploadedFileInterface;

// ARRANGE: Create a stream with test content
$content = 'PSR-17 factory test content';
$stream = Stream::fromString($content);

// ACT: Create UploadedFile via PSR-17 factory
$uploadedFile = UploadedFile::create(
    $stream,
    strlen($content),
    UPLOAD_ERR_OK,
    'factory-test.txt',
    'text/plain'
);

// ASSERT: Implements interface
echo "Implements UploadedFileInterface: ";
var_dump($uploadedFile instanceof UploadedFileInterface);

// ASSERT: Metadata is correct
echo "Size: ";
var_dump($uploadedFile->getSize() === strlen($content));

echo "Error: ";
var_dump($uploadedFile->getError() === UPLOAD_ERR_OK);

echo "Client filename: ";
var_dump($uploadedFile->getClientFilename() === 'factory-test.txt');

echo "Client media type: ";
var_dump($uploadedFile->getClientMediaType() === 'text/plain');

// ASSERT: Can get stream
$stream2 = $uploadedFile->getStream();
echo "Stream accessible: ";
var_dump($stream2 !== null);

echo "PSR-17 create() tests passed\n";
?>
--CLEAN--
<?php
gc_collect_cycles();
?>
--EXPECT--
Implements UploadedFileInterface: bool(true)
Size: bool(true)
Error: bool(true)
Client filename: bool(true)
Client media type: bool(true)
Stream accessible: bool(true)
PSR-17 create() tests passed
