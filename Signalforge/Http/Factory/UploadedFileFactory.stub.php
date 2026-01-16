<?php
/**
 * Signalforge HTTP Extension
 * UploadedFileFactory.stub.php - IDE stub for UploadedFileFactory class
 *
 * @package Signalforge\Http
 */

namespace Signalforge\NativeHttp;

/**
 * PSR-17 compliant uploaded file factory.
 *
 * Creates UploadedFileInterface instances.
 *
 * @final
 * @implements \Psr\Http\Message\UploadedFileFactoryInterface
 */
final class UploadedFileFactory implements \Psr\Http\Message\UploadedFileFactoryInterface
{
    /**
     * Create a new uploaded file.
     *
     * @param \Psr\Http\Message\StreamInterface $stream Underlying stream representing the uploaded file
     * @param int|null $size Size in bytes
     * @param int $error PHP file upload error constant (UPLOAD_ERR_OK by default)
     * @param string|null $clientFilename Filename as provided by the client, if any
     * @param string|null $clientMediaType Media type as provided by the client, if any
     * @return \Psr\Http\Message\UploadedFileInterface
     * @throws \InvalidArgumentException If the file resource is not valid
     */
    public function createUploadedFile(
        \Psr\Http\Message\StreamInterface $stream,
        ?int $size = null,
        int $error = \UPLOAD_ERR_OK,
        ?string $clientFilename = null,
        ?string $clientMediaType = null
    ): \Psr\Http\Message\UploadedFileInterface {}
}
