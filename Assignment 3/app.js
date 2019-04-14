    'use strict'

// C library API
const ffi = require('ffi');

// Express App (Routes)
const express = require("express");
const app     = express();
const path    = require("path");
const fileUpload = require('express-fileupload');

app.use(fileUpload());

// Minimization
const fs = require('fs');
const JavaScriptObfuscator = require('javascript-obfuscator');

// Important, pass in port as in `npm run dev 1234`, do not change
const portNum = process.argv[2];

// Send HTML at root, do not change
app.get('/',function(req,res){
  res.sendFile(path.join(__dirname+'/public/index.html'));
});

// Send Style, do not change
app.get('/style.css',function(req,res){
  //Feel free to change the contents of style.css to prettify your Web app
  res.sendFile(path.join(__dirname+'/public/style.css'));
});

// Send obfuscated JS, do not change
app.get('/index.js',function(req,res){
  fs.readFile(path.join(__dirname+'/public/index.js'), 'utf8', function(err, contents) {
    const minimizedContents = JavaScriptObfuscator.obfuscate(contents, {compact: true, controlFlowFlattening: true});
    res.contentType('application/javascript');
    res.send(minimizedContents._obfuscatedCode);
  });
});

//Respond to POST requests that upload files to uploads/ directory
app.post('/upload', function(req, res) {
  if(!req.files) {
    return res.status(400).send('No files were uploaded.');
  }

  let uploadFile = req.files.uploadFile;

  if(uploadFile.name.split('.').pop() == "ics") {
  // Use the mv() method to place the file somewhere on your server
      uploadFile.mv(__dirname+'/uploads/' + uploadFile.name, function(err) {
        if(err) {
          return res.status(500).send(err);
        }
        res.redirect('/');
      });
    }
});

//Respond to GET requests for files in the uploads/ directory
app.get('/uploads/:name', function(req , res){
  fs.stat(__dirname+'/uploads/' + req.params.name, function(err, stat) {
    console.log(err);
    if(err == null) {
      res.sendFile(path.join(__dirname+'/uploads/' + req.params.name));
    } else {
      res.send('');
    }
  });
});

//******************** Your code goes here ********************
const ref = require('ref');
const Struct = require('ref-struct');

//Pointers to icalendar types (and char)
var charPtr = ref.refType('char');
var cstring = ref.types.CString;

//C library interface
var libParser = ffi.Library('./parser.so', {
    'getAlarmListJSON':['void', ['string', 'string', cstring]],
    'getCalendarJSON':['void', ['string', cstring]],
    'getEventListJSON':['void', ['string', cstring]],
    'getOptionalPropertiesJSON':['void', ['string', 'string', cstring]],
    'addEventToCalendar':['void', ['string', 'string', cstring]],
    'createUserCalendar':['void', ['string', 'string', 'string', cstring]],
});
//Get array of file names for the dropdown
app.get('/fileList', function(req, res) {
    var fileArray = fs.readdirSync(__dirname+'/uploads');
    var fileJSON = JSON.stringify(fileArray);
    res.send(fileJSON);
});

//Get table of file data, e.g. calendar link, version number, etc.
//Formats it as html on the server to use the fileArray and the calendarJSON
//at the same time, which was too convenient not to use.
app.get('/fileTable', function(req, res) {
    var calendarJSON = [];
    var fileArray = fs.readdirSync(__dirname+'/uploads');
    fileArray.forEach(file => {
        var jsonBuffer = new Buffer(500);
        libParser.getCalendarJSON(file, jsonBuffer);
        calendarJSON.push(JSON.parse(ref.readCString(jsonBuffer)));
    });

    let fileTable = '';
    if(fileArray.length == 0) {
        fileTable += "<strong>No files.</strong>";
    } else {
        fileTable += "<table><tr><th>File name (click to download)</th><th>Version</th><th>Product ID</th><th>Number of events</th><th>Number of properties</th></tr>";
        for(var i = 0; i < fileArray.length; i++) {
            fileTable+='<tr><td><a href="/uploads/'+fileArray[i]+'">'+fileArray[i]+'</a></td>';
            //handle errors:
            if(calendarJSON[i].hasOwnProperty("error")) {
                fileTable+='<td></td><td>Invalid file</td><td></td><td></td>';
            } else {
                fileTable+='<td>'+calendarJSON[i].version+'</td><td>'+calendarJSON[i].prodID+'</td><td>'+calendarJSON[i].numEvents+'</td><td>'+calendarJSON[i].numProps+'</td>';
            }
            fileTable+='</tr>';
        }
        fileTable+="</table>";
    }
    res.send(fileTable);
});

//Get table of event data, e.g. start date and time, summary, etc.
app.get('/eventTable', function(req, res) {
    //determine specific calendar first
    const calendarName = req.query.calendarName;
    let eventListBuffer = new Buffer(2000);
    libParser.getEventListJSON(calendarName, eventListBuffer);
    let eventListJSON = JSON.parse(ref.readCString(eventListBuffer));
    res.send(eventListJSON);
});

//Get alarm JSON
app.get('/alarmJSON', function(req, res) {
    //determine specific calendar first
    const calendarName = req.query.info.split('_')[0];
    const eventID = req.query.info.split('_')[1];
    let alarmListBuffer = new Buffer(2000);
    libParser.getAlarmListJSON(calendarName, eventID, alarmListBuffer);
    let alarmListJSON = JSON.parse(ref.readCString(alarmListBuffer));
    //console.log(alarmListJSON);
    res.send(alarmListJSON);
});

//Get property JSON from an event
app.get('/propertyJSON', function(req, res) {
    //determine specific calendar first
    const calendarName = req.query.info.split('_')[0];
    const eventID = req.query.info.split('_')[1];
    let propertyListBuffer = new Buffer(2000);
    libParser.getOptionalPropertiesJSON(calendarName, eventID, propertyListBuffer);
    let propertyListJSON = JSON.parse(ref.readCString(propertyListBuffer));
    //console.log(propertyListJSON);
    res.send(propertyListJSON);
});

//Add event to selected calendar
app.get('/addEvent', function(req, res) {
    const data = req.query;
    let eventJSON = {
        createDT:{
            date:data.creationDate,
            time:data.creationTime,
            isUTC:data.creationUTC
        },
        startDT:{
            date:data.startDate,
            time:data.startTime,
            isUTC:data.startUTC
        },
        uid:data.uid,
        summary:data.summary
    }
    var buffer = new Buffer(200);
    //console.log(eventJSON);
    libParser.addEventToCalendar(data.calendarName, JSON.stringify(eventJSON), buffer);
    res.send(ref.readCString(buffer));
});

app.get('/createCalendar', function(req, res) {
    const data = req.query;
    let calendarJSON = {
        version:data.version,
        prodID:data.prodID
    }
    let eventJSON = {
        createDT:{
            date:data.creationDate,
            time:data.creationTime,
            isUTC:data.creationUTC
        },
        startDT:{
            date:data.startDate,
            time:data.startTime,
            isUTC:data.startUTC
        },
        uid:data.uid,
        summary:data.summary
    }
    var buffer = new Buffer(200);
    libParser.createUserCalendar(data.fileName, JSON.stringify(calendarJSON), JSON.stringify(eventJSON), buffer);
    res.send(ref.readCString(buffer));
});

//Sample endpoint
app.get('/someendpoint', function(req , res){
  res.send({
    foo: "bar"
  });
});

app.listen(portNum);
console.log('Running app at localhost: ' + portNum);
