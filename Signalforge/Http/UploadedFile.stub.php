<?php
/**
 * Signalforge HTTP Extension
 * UploadedFile.stub.php - IDE stub for UploadedFile class
 *
 * @package Signalforge\Http
 */

namespace Signalforge\NativeHttp;

/**
 * High-performance HTTP UploadedFile handler implementing PSR-7 UploadedFileInterface.
 *
 * Provides efficient file upload handling with zero-copy operations.
 *
 * @final
 * @implements \Psr\Http\Message\UploadedFileInterface
 */
final class UploadedFile implements \Psr\Http\Message\UploadedFileInterface
{
    /**
     * Private constructor - use factory methods or Request::getUploadedFiles().
     */
    private function __construct() {}

    /* ============================================================================
     * PSR-7 UploadedFileInterface
     * ============================================================================ */

    /**
     * Retrieve a stream representing the uploaded file.
     *
     * @return \Psr\Http\Message\StreamInterface Stream representation of the uploaded file
     * @throws \RuntimeException in cases when no stream is available or can be constructed
     */
    public function getStream(): \Psr\Http\Message\StreamInterface {}

    /**
     * Move the uploaded file to a new location.
     *
     * @param string $targetPath Path to which to move the uploaded file
     * @return void
     * @throws \InvalidArgumentException if the $targetPath specified is invalid
     * @throws \RuntimeException on any error during the move operation
     */
    public function moveTo(string $targetPath): void {}

    /**
     * Retrieve the file size.
     *
     * @return int|null The file size in bytes or null if unknown
     */
    public function getSize(): ?int {}

    /**
     * Retrieve the error associated with the uploaded file.
     *
     * @return int One of PHP's UPLOAD_ERR_XXX constants
     */
    public function getError(): int {}

    /**
     * Retrieve the filename sent by the client.
     *
     * @return string|null The filename sent by the client or null if none was provided
     */
    public function getClientFilename(): ?string {}

    /**
     * Retrieve the media type sent by the client.
     *
     * @return string|null The media type sent by the client or null if none was provided
     */
    public function getClientMediaType(): ?string {}
}

