--TEST--
UploadedFile object lifecycle - file path and metadata management
--EXTENSIONS--
signalforge_http
--FILE--
<?php
use Signalforge\NativeHttp\UploadedFile;
use Signalforge\NativeHttp\Request;

// ARRANGE: Test UploadedFile memory management via Request::capture()

echo "=== Test 1: Multiple UploadedFile via Request::capture() ===\n";
for ($i = 0; $i < 10; $i++) {
    $_SERVER = ['REQUEST_METHOD' => 'POST', 'REQUEST_URI' => '/test'];
    $tmpfile = tempnam(sys_get_temp_dir(), "upload_{$i}_");
    file_put_contents($tmpfile, "Content {$i}");

    $_FILES = [
        "file{$i}" => [
            'name' => "test{$i}.txt",
            'type' => 'text/plain',
            'tmp_name' => $tmpfile,
            'error' => UPLOAD_ERR_OK,
            'size' => strlen("Content {$i}"),
        ]
    ];

    $request = Request::capture();
    $files = $request->getUploadedFiles();
    $upload = $files["file{$i}"];

    // ASSERT: File should be accessible
    var_dump($upload->getClientFilename() === "test{$i}.txt");
    var_dump($upload->getSize() === strlen("Content {$i}"));

    @unlink($tmpfile);
}
echo "Request capture lifecycle passed\n";

echo "\n=== Test 2: UploadedFile metadata operations ===\n";
$_SERVER = ['REQUEST_METHOD' => 'POST', 'REQUEST_URI' => '/upload'];
$tmpfile = tempnam(sys_get_temp_dir(), 'metadata_');
file_put_contents($tmpfile, 'Test content for metadata');

$_FILES = [
    'document' => [
        'name' => 'report.pdf',
        'type' => 'application/pdf',
        'tmp_name' => $tmpfile,
        'error' => UPLOAD_ERR_OK,
        'size' => strlen('Test content for metadata'),
    ]
];

$request = Request::capture();
$files = $request->getUploadedFiles();
$upload = $files['document'];

// ASSERT: Metadata should be correct
var_dump($upload->getClientFilename() === 'report.pdf');
var_dump($upload->getClientMediaType() === 'application/pdf');
var_dump($upload->getError() === UPLOAD_ERR_OK);
var_dump($upload->getSize() === strlen('Test content for metadata'));

@unlink($tmpfile);
echo "Metadata operations passed\n";

echo "\n=== Test 3: Multiple files in single request ===\n";
$_SERVER = ['REQUEST_METHOD' => 'POST', 'REQUEST_URI' => '/multi-upload'];
$files_data = [];

for ($i = 0; $i < 5; $i++) {
    $tmpfile = tempnam(sys_get_temp_dir(), "multi_{$i}_");
    file_put_contents($tmpfile, "Multi content {$i}");

    $files_data["upload{$i}"] = [
        'name' => "file{$i}.txt",
        'type' => 'text/plain',
        'tmp_name' => $tmpfile,
        'error' => UPLOAD_ERR_OK,
        'size' => strlen("Multi content {$i}"),
    ];
}

$_FILES = $files_data;
$request = Request::capture();
$uploads = $request->getUploadedFiles();

// ASSERT: All files should be present
foreach ($files_data as $key => $file_info) {
    var_dump(isset($uploads[$key]));
    var_dump($uploads[$key]->getClientFilename() === $file_info['name']);
    @unlink($file_info['tmp_name']);
}

echo "Multiple files passed\n";

echo "\n=== Test 4: Rapid Request creation/destruction with files ===\n";
for ($i = 0; $i < 20; $i++) {
    $_SERVER = ['REQUEST_METHOD' => 'POST', 'REQUEST_URI' => '/rapid'];
    $tmpfile = tempnam(sys_get_temp_dir(), "rapid_{$i}_");
    file_put_contents($tmpfile, "Rapid {$i}");

    $_FILES = [
        'rapid' => [
            'name' => "rapid{$i}.txt",
            'type' => 'text/plain',
            'tmp_name' => $tmpfile,
            'error' => UPLOAD_ERR_OK,
            'size' => strlen("Rapid {$i}"),
        ]
    ];

    $req = Request::capture();
    $files = $req->getUploadedFiles();
    $upload = $files['rapid'];
    $name = $upload->getClientFilename();

    @unlink($tmpfile);
    // Request and UploadedFile destroyed at end of loop
}
echo "Rapid lifecycle passed\n";

echo "\n=== Test 5: Error handling for upload errors ===\n";
$_SERVER = ['REQUEST_METHOD' => 'POST', 'REQUEST_URI' => '/error-test'];
$tmpfile = tempnam(sys_get_temp_dir(), 'error_');
file_put_contents($tmpfile, 'Error test');

$_FILES = [
    'error_file' => [
        'name' => 'error.txt',
        'type' => 'text/plain',
        'tmp_name' => $tmpfile,
        'error' => UPLOAD_ERR_CANT_WRITE,
        'size' => 10,
    ]
];

$request = Request::capture();
$files = $request->getUploadedFiles();
$upload = $files['error_file'];

// ASSERT: Error should be preserved
var_dump($upload->getError() === UPLOAD_ERR_CANT_WRITE);

@unlink($tmpfile);
echo "Error handling passed\n";

echo "\nAll UploadedFile ownership tests passed\n";
?>
--EXPECT--
=== Test 1: Multiple UploadedFile via Request::capture() ===
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
bool(true)
Request capture lifecycle passed

=== Test 2: UploadedFile metadata operations ===
bool(true)
bool(true)
bool(true)
bool(true)
Metadata operations passed

=== Test 3: Multiple files in single request ===
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
Multiple files passed

=== Test 4: Rapid Request creation/destruction with files ===
Rapid lifecycle passed

=== Test 5: Error handling for upload errors ===
bool(true)
Error handling passed

All UploadedFile ownership tests passed
