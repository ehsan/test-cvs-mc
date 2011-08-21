/**
 * Expects a file. Returns an object containing the size, type, name and path
 * using another worker. Used to test posting of file from worker to worker.
 */
onmessage = function(event) {
  var worker = new Worker("file_worker.js");

  worker.postMessage(event.data);

  worker.onmessage = function(event) {
    postMessage(event.data);
  }

  worker.onerror = function(event) {
    postMessage(undefined);
  }
};
