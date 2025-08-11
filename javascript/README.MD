# Iris Codec Javascript API
[![Iris Codec WASM CI](https://img.shields.io/github/actions/workflow/status/IrisDigitalPathology/Iris-Codec/emcmake-wasm-CI.yml?style=for-the-badge&logo=github&label=Iris%20Codec%20WebAssembly%20CI)](https://github.com/IrisDigitalPathology/Iris-Codec/actions/workflows/cmake-macos-CI.yml)
[![NPM Version](https://img.shields.io/npm/v/iris-codec?style=for-the-badge&logo=npm)](https://www.npmjs.com/package/iris-codec)
[![jsDelivr hits (npm)](https://img.shields.io/jsdelivr/npm/hw/iris-codec?style=for-the-badge&logo=jsDelivr&link=https%3A%2F%2Fcdn.jsdelivr.net%2Fnpm%2Firis-codec%2F)](https://www.jsdelivr.com/package/npm/iris-codec)

Copyright &copy; 2025 Iris Developers; MIT Software License

<!-- <svg xmlns="http://www.w3.org/2000/svg" width="162.4" height="48" viewBox="0 -20 406 120" fill="none"><path d="M91.2 9.4H10.6c-.9 0-1.7.8-1.7 1.7v58.8c0 .9.8 1.7 1.7 1.7h16c.9 0 1.7.8 1.7 1.7v14.8c0 1.5 1.8 2.3 2.9 1.2l18-17.3c.3-.3.7-.5 1.2-.5h40.9c.9 0 1.7-.8 1.7-1.7V11.1c0-.9-.8-1.7-1.7-1.7Z" stroke="#bf3939" stroke-width="8" stroke-miterlimit="10"/><text x="43.4" y="60.7" font-family="sans-serif" font-size="48" fill="#bf3939">!</text><text x="118" y="75" font-family="sans-serif" font-size="66" fill="#bf3939">Important</text></svg> --> 
> ![Important Icon](data:image/svg+xml;base64,PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHdpZHRoPSIxNjIuNCIgaGVpZ2h0PSI0OCIgdmlld0JveD0iMCAtMjAgNDA2IDEyMCIgZmlsbD0ibm9uZSI+PHBhdGggZD0iTTkxLjIgOS40SDEwLjZjLS45IDAtMS43LjgtMS43IDEuN3Y1OC44YzAgLjkuOCAxLjcgMS43IDEuN2gxNmMuOSAwIDEuNy44IDEuNyAxLjd2MTQuOGMwIDEuNSAxLjggMi4zIDIuOSAxLjJsMTgtMTcuM2MuMy0uMy43LS41IDEuMi0uNWg0MC45Yy45IDAgMS43LS44IDEuNy0xLjdWMTEuMWMwLS45LS44LTEuNy0xLjctMS43WiIgc3Ryb2tlPSIjYmYzOTM5IiBzdHJva2Utd2lkdGg9IjgiIHN0cm9rZS1taXRlcmxpbWl0PSIxMCIvPjx0ZXh0IHg9IjQzLjQiIHk9IjYwLjciIGZvbnQtZmFtaWx5PSJzYW5zLXNlcmlmIiBmb250LXNpemU9IjQ4IiBmaWxsPSIjYmYzOTM5Ij4hPC90ZXh0Pjx0ZXh0IHg9IjExOCIgeT0iNzUiIGZvbnQtZmFtaWx5PSJzYW5zLXNlcmlmIiBmb250LXNpemU9IjY2IiBmaWxsPSIjYmYzOTM5Ij5JbXBvcnRhbnQ8L3RleHQ+PC9zdmc+)\
> We provide two mechanisms to use JavaScript to access Iris Encoded Files. <b><u>Iris RESTful Server is by far the preferred method</u></b>. The WASM module on NPM (and jsDelivr) is a convenience method that allows in-browser deserializating of the Iris bytestream for compatability with simple static HTTP file serving services like Amazon S3 or Google Cloud Storage -- but it comes at a cost in initial file load time and a more challenging API. **This is a feature not offered by other whole slide image (WSI) formats**. It is challenging. <u>**Read this document carefully if you chose to use this unique feature**</u> and <u>design your viewers to simultaneously open all slides of a case</u> when a case is opened. There is very little memory overhead (1-2 MB per slide) and you will only experience the offset table generation delay once. 

The two RESTful compatible options are as follows:
1. <strong>[Iris RESTful Server]()</strong>: This is the most efficient method of streaming image data to a client. This is a <u>**server side serialization**</u> tool. It will map Iris Slides into virtual memory and return image data quickly. This is only compatible with HTTP servers that have a true file system. This solution is very easy to implement using the [OpenSeaDragon IrisTileSource](https://github.com/openseadragon/openseadragon/blob/master/src/iristilesource.js) in about 4 lines of code (see below).
2. <strong>[Iris-Codec WebAssembly Module](https://www.npmjs.com/package/iris-codec)</strong>: This is a slower method of streaming image data. This is a <u>**client side serialization**</u> tool.  The major benefit is that this is fully compatible with bucked-based storage solutions such as AWS S3 or Google Cloud Storage. It is slower because it validates and loads the full slide metadata in a series of small HTTP 206 'partial read' responses (including offset tables) upfront; following validation and offset generation however, reads are fast as they use HTTP ranged reads for tile data, hitting the same bucket file over and over again. Because of this limitation, we **STRONGLY** recommend <u>abstracting the metadata for all slides within a case set simultaneously</u> as it requires a small footprint (1-2 MB/slide) and allows for fast access following this file metadata abstraction. 

## Installation
### Installation of Iris RESTful Server
Please refer to the <strong>[Iris RESTful Server documentation](https://github.com/IrisDigitalPathology/Iris-RESTful-Server?tab=readme-ov-file#deployment-introduction)</strong> for a description of deploying the Iris RESTful Server. The easiest way is through deployment of our [official container images](https://ghcr.io/irisdigitalpathology/iris-restful:latest).

### Installation of Iris-Codec WebAssembly
When used in a client side system, jsDelivr will distribute the WebAssembly module from NPM. There is no installation needed from you.
```html
<!DOCTYPE html>
<script type="module">
    import createModule 
    from 'https://cdn.jsdelivr.net/npm/iris-codec@latest/iris-codec.js';
    
    // Compile the WASM module and stall execution until ready
    const irisCodec = await createModule();
    console.log("Iris-Codec has been loaded");
    
    // ...Your code goes here...
</script>
```

The general use case is in web applications. If you wish to use it with NodeJS to access remote slides or for node based testing it can be installed from NPM. Please do not try to build a server with this. It is designed for client-side applications and will bottleneck your server.

```sh
npm i iris-codec
```

<!-- <svg xmlns="http://www.w3.org/2000/svg" width="162.4" height="48" viewBox="3 -6.6 135 36"><polygon points="27.3 3.4 17.8 3.4 11.1 10.1 11.1 19.6 17.8 26.4 27.3 26.4 34 19.6 34 10.1 27.3 3.4" fill="none" stroke="#bf3939" stroke-width="3" stroke-miterlimit="4"/><text transform="translate(19.9 20.8)" font-family="sans-serif" font-size="18"  fill="#bf3939">!</text><text transform="translate(49.7 23.2)" font-family="sans-serif" font-size="24"  fill="#bf3939">Caution</text></svg> -->
> ![Caution Icon](data:image/svg+xml;base64,PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHdpZHRoPSIxNjIuNCIgaGVpZ2h0PSI0OCIgdmlld0JveD0iMyAtNi42IDEzNSAzNiI+PHBvbHlnb24gcG9pbnRzPSIyNy4zIDMuNCAxNy44IDMuNCAxMS4xIDEwLjEgMTEuMSAxOS42IDE3LjggMjYuNCAyNy4zIDI2LjQgMzQgMTkuNiAzNCAxMC4xIDI3LjMgMy40IiBmaWxsPSJub25lIiBzdHJva2U9IiNiZjM5MzkiIHN0cm9rZS13aWR0aD0iMyIgc3Ryb2tlLW1pdGVybGltaXQ9IjQiLz48dGV4dCB0cmFuc2Zvcm09InRyYW5zbGF0ZSgxOS45IDIwLjgpIiBmb250LWZhbWlseT0ic2Fucy1zZXJpZiIgZm9udC1zaXplPSIxOCIgIGZpbGw9IiNiZjM5MzkiPiE8L3RleHQ+PHRleHQgdHJhbnNmb3JtPSJ0cmFuc2xhdGUoNDkuNyAyMy4yKSIgZm9udC1mYW1pbHk9InNhbnMtc2VyaWYiIGZvbnQtc2l6ZT0iMjQiICBmaWxsPSIjYmYzOTM5Ij5DYXV0aW9uPC90ZXh0Pjwvc3ZnPg==)\
> Do not use this package from NPM for a server deployment that serves Iris encoded slides! This is a client slide tool and **the webassembly module was not designed for use in a server** and it will not work well for this purpose. Trust me. If you need a server, <strong>use Iris RESTful server instead</strong>; frankly, it is significantly faster and more robust than what Node will give you for slide tile serving.


## Implementing JavaScript API
### Iris RESTful API
Iris RESTful has a simple API (and supports DICOMweb WADO-RS), outlined here, and explained in greater detail within the [Iris RESTful API Exlained Section](https://github.com/IrisDigitalPathology/Iris-RESTful-Server/blob/main/README.md#api-explained). This will return the slide metadata in JSON format and slide tile image data as an image blob for direct use as an image element source.

```
Iris RESTful
GET <URL>/slides/<slide-name>/metadata
GET <URL>/slides/<slide-name>/layers/<layer>/tiles/<tile>

Supported WADO-RS
GET <URL>/studies/<study>/series/<UID>/metadata
GET <URL>/studies/<study>/series/<UID>/instances/<layer>/metadata
GET <URL>/studies/<study>/series/<UID>/instances/<layer>/frames/<tile>
```


### Iris JavaScript WASM API
Load the Iris-Codec NPM WebAssembly module via jsDelivr (or download the [latest javascript release](https://github.com/IrisDigitalPathology/Iris-Codec/releases/latest) and include your local copy)
```html
<!DOCTYPE html>
<script type="module">
    import createModule 
    from 'https://cdn.jsdelivr.net/npm/iris-codec@latest/iris-codec.js';
    
    // Compile the WASM module and stall execution until ready
    const irisCodec = await createModule();
    console.log("Iris-Codec has been loaded");
    
    // ...Your code goes here...
</script>
```

Once loaded, you can access image data in a manner similar to the C++ and Python Iris-Codec API. Importantly, the metadata will be returned in IrisCodec::Abstraction C++ types (file metadata abstractions) exposed using Emscripten bindings. Refer to the included TypeScript file for those definitions. Image tile data will be returned as an image (MIME) source and can be used directly as an image data source.

We support callback notation presently as it is both common/well established with JS programmers and easy to construct promises from a callback; however it is more challening to reformat a promise into a callback structure for legacy support. We may simply move to promises in the future. If you wish to wrap file access in promise structures, here are example definitions:
```js
// Wraps Module.validateFileStructure (url, callback) in a Promise
function validateFileStructureAsync(Module, fileUrl) {
  return new Promise((resolve, reject) => {
    Module.validateFileStructure(fileUrl, (result) => {
        const SUCCESS = Module.ResultFlag.IRIS_SUCCESS.value;
        if (result.flag.value === SUCCESS) {
            resolve();
        } else {
            // result.message is guaranteed to be a JS string
            reject(new Error(result.message));
        }
    });
  });
}
// Wraps Module.openIrisSlide(url, callback) in a Promise
function openIrisSlideAsync(url) {
  return new Promise((resolve, reject) => {
    Module.openIrisSlide(url, slide => {
      if (!slide) {
        reject(new Error("Failed to validate"));
      } else {
        resolve(slide);
      }
    });
  });
}
// Wraps slide.getSlideTile(layer, tileIndex, callback) in a Promise
function getSlideTileAsync(slide, layer, tileIndex) {
  return new Promise((resolve, reject) => {
    slide.getSlideTile(layer, tileIndex, tile_image => {
      if (tile_image) {
        resolve(tile_image);
      } else {
        reject(new Error("Failed to get tile"));
      }
    });
  });
}
```

Perform a deep validation of the slide file structure. This will navigate the internal offset-chain and check for violations of the IFE standard. This can be omitted if you are confident of the source.

```js
const irisCodec = await createModule();
console.log("Iris-Codec has been loaded");
const url = "https://irisdigitalpathology.s3.us-east-2.amazonaws.com/example-slides/cervix_2x_jpeg.iris";
try {
    await validateFileStructureAsync(irisCodec, url);
    console.log(`Slide file at ${url} successfully passed validation`);
} catch (error) {
    console.log(`Slide file at ${url} failed validation: ${error}`);
}
```
Open a slide file. The following conditional will succeed without throwing an exception if the slide has already passed validation but you may skip validation to reduce server requests.
```js
const irisCodec = await createModule();
console.log("Iris-Codec has been loaded");
const url = "https://irisdigitalpathology.s3.us-east-2.amazonaws.com/example-slides/cervix_2x_jpeg.iris";
try {
  await validateFileStructureAsync(irisCodec,url);
  const slide = await openIrisSlideAsync(irisCodec, url);

  // ...Do something with slide

  slide.delete();
} catch (error) {
  console.error(error);
}
```
Get the slide abstraction, read off the slide dimensions, and then print it to the console.
```js
const irisCodec = await createModule();
const url = "https://irisdigitalpathology.s3.us-east-2.amazonaws.com/example-slides/cervix_4x_jpeg.iris";
try {
  await validateFileStructureAsync(irisCodec,url);
  const slide = await openIrisSlideAsync (irisCodec, url);

  // Let's get the slide dimensions and print them to the console.
  const info = slide.getSlideInfo();
  const extent = info.extent;
  console.log(`Slide file ${extent.width} px by ${extent.height}px at lowest resolution layer. The layer extents are as follows:`);
  console.log(`There are ${extent.layers.size()} layers comprising the following dimensions:`)
  for (var i = 0; i < extent.layers.size(); i++) {
      const layer = extent.layers.get(i);
      console.log(`  Layer ${i}: ${layer.xTiles} x-tiles, ${layer.xTiles} y-tiles, ${layer.scale}x scale`);
  }
  slide.delete();
} catch (error) {
  console.error(error);
}
```
Generate a quick view of the one of the images (`tileImage`) somewhere earlier in the HTML page.
```html
<img id="tileImage" width="128" height="128" alt="Loading..." style="border: 1px solid black;"/>
<!-- ... Somewhere Earlier -->
<script type="module">
    // Earlier Promise definitions 
    try {
      await validateFileStructureAsync(irisCodec,url);
      const slide = await openIrisSlideAsync (irisCodec, url);
      const layer = 0;
      const tile = 0;
      const tileData = await getSlideTileAsync(slide, layer, tile);

      // Now pass the image off to the 'tileImage' element.
      const url = URL.createObjectURL(tileData);
      const imgElement = document.getElementById("tileImage");
      imgElement.src = url;

      // Clean up after the image is loaded
      imgElement.onload = () => {
          URL.revokeObjectURL(objectUrl);
          slide.delete();
      };
    } catch (error) {
      console.error(error);
    }
</script>
```

Bringing it all together, the following full HTML page source will show a low power view of the image using a tile grid view similar to the view Pillow.Images produces in the [Python API example](https://github.com/IrisDigitalPathology/Iris-Codec?tab=readme-ov-file#python-example-api).
```html
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <title>Iris-Codec WASM: Tile Grid</title>

  <style>
    /* The grid container: auto-flows row by row */
    #tileGrid {
      display: grid;
      justify-content: start;      /* left-align if container is wide */
      align-content: start;        /* top-align if container is tall */
    }
    #tileGrid img {
      width: 128px;
      height: 128px;
      object-fit: cover;           /* crop or letterbox if needed */
      background: #f0f0f0;         /* placeholder color */
    }
  </style>
</head>

<body>
  <h1>Iris-Codec WASM: Tile Grid</h1>
  <div id="loadTimeText">Load time: —</div>
  <div id="tileGrid"></div>

  <script type="module">
  import createModule from './iris-codec.js';
  // ————— Helper Promisified APIs —————
  function validateFileStructureAsync(Module, fileUrl) {
    return new Promise((resolve, reject) => {
      Module.validateFileStructure(fileUrl, (result) => {
        const SUCCESS = Module.ResultFlag.IRIS_SUCCESS.value;
        if (result.flag.value === SUCCESS) resolve();
        else reject(new Error(result.message));
      });
    });
  }
  function openIrisSlideAsync(Module, url) {
    return new Promise((resolve, reject) => {
      Module.openIrisSlide(url, slide => {
        slide ? resolve(slide)
              : reject(new Error("Failed to open slide"));
      });
    });
  }
  function getSlideTileAsync(slide, layer, tileIndex) {
    return new Promise((resolve, reject) => {
      slide.getSlideTile(layer, tileIndex, tile_blob => {
        tile_blob ? resolve(tile_blob)
                  : reject(new Error("Failed to get tile “" + tileIndex + "”"));
      });
    });
  }
  
  // ————— Main Entry Point —————
  (async () => {
    const irisCodec = await createModule();
    const url       = "https://irisdigitalpathology.s3.us-east-2.amazonaws.com/example-slides/cervix_4x_jpeg.iris";
    try {
      await validateFileStructureAsync(irisCodec,url);
      const slide = await openIrisSlideAsync(irisCodec, url);

      // 1) Read layer info
      const layer         = 1;
      const info          = slide.getSlideInfo();
      const extent        = info.extent.layers.get(layer);
      const { xTiles, yTiles } = extent;
      const nTiles        = xTiles * yTiles;

      // 2) Configure the grid container
      const gridEl = document.getElementById("tileGrid");
      gridEl.style.gridTemplateColumns = `repeat(${xTiles}, 128px)`;
      gridEl.style.gridTemplateRows    = `repeat(${yTiles}, 128px)`;

      // 3) Fetch all tiles in parallel (or chunked if you prefer)
      const startAll = performance.now();
      const objectUrls = await Promise.all(
        Array.from({ length: nTiles }, (_, idx) =>
          getSlideTileAsync(slide, layer, idx)
            .then(blob => URL.createObjectURL(blob))
        )
      );
      const endAll = performance.now();
      document.getElementById("loadTimeText").textContent =
        `All tiles fetched in ${Math.round(endAll - startAll)} ms`;

      // 4) Insert <img> elements in tile order
      objectUrls.forEach(u => {
        const img = new Image(128, 128);
        img.src = u;
        gridEl.appendChild(img);
      });

      // 5) Cleanup on unload
      window.addEventListener("beforeunload", () => {
        // Revoke all blob URLs
        objectUrls.forEach(u => URL.revokeObjectURL(u));
        // Free the slide
        slide.delete();
      });

    } catch (err) {
      console.error(err);
      alert("Error: " + err.message);
    }
  })();
  </script>
</body>
</html>
```
<!-- result, info = slide.get_info()
if (result.success() == False):
    raise Exception(f'Failed to read slide information: {result.message()}') -->

