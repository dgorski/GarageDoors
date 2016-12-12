== SPIFFs filesystem upload directory

There is an Arduino IDE plugin for the ESP8266 boards to upload files to the SPIFFs filesystem area in the on-board EEPROM:

https://github.com/esp8266/Arduino/blob/master/doc/filesystem.md#uploading-files-to-file-system

This data directory is used during the build phase.

Simply copy the contents of the webapp folder here and use the IDE Tools menu item to upload files to the ESP8266.

You can compress files with .gz and the internal webserver will send them as content-encoded, for example:

  garage.html -> garage.html.gz

The filesytem will answer requests for garage.html by returning garage.html.gz if it is present, with a content-transfer-encoding of x-gzip.  This reduces file size on the device, and download time for clients.

I generally compress the html, js and css files, but leave images (png, etc) as-is, they contain compression already.

You can delete the non-gz version of the files from the data dir before uploading, the non-compresses version does not need to be present for this to work.
