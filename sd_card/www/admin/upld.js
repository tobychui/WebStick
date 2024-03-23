/*
    Upld.js

    This script can be included in any other pages
    to provide file upload functions

    Usage: handleFile(file, dir, callback)
    E.g.
    const blob = new Blob(["Hello World!"], { type: 'text/plain' });
    const file = new File([blob], "hello.txt");
    handleFile(file, "/down/", function(){
        alert("Upload done");
    });
*/
function handleFile(file, dir, callback=undefined) {
    // Perform actions with the selected file
    console.log('Uploading file:', file);
    var formdata = new FormData();
    formdata.append("file1", file);
    var ajax = new XMLHttpRequest();
    ajax.upload.addEventListener("progress", progressHandler, false);
    ajax.addEventListener("load", function(event){
        completeHandler(event);
        if (callback != undefined){
            callback();
        }
    }, false); // doesnt appear to ever get called even upon success
    ajax.addEventListener("error", errorHandler, false);
    ajax.addEventListener("abort", abortHandler, false);
    ajax.open("POST", "/upload?dir=" + dir);
    ajax.send(formdata);
}

function progressHandler(event) {
    //_("loaded_n_total").innerHTML = "Uploaded " + event.loaded + " bytes of " + event.total; // event.total doesnt show accurate total file size
   
    var percent = (event.loaded / event.total) * 100;
    $("#uploadProgressBar").find(".bar").css("width", Math.round(percent) + "%");
    console.log("Uploaded " + event.loaded + " bytes => " + percent +"%");
    if (percent >= 100) {
        $("#uploadProgressBar").find(".bar").css("width", "100%");
        //_("status").innerHTML = "Please wait, writing file to filesystem";
    }
}
function completeHandler(event) {
}


function errorHandler(event) {
}

function abortHandler(event) {

}