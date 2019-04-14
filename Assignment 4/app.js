    'use strict'

// C library API
const ffi = require('ffi');

// Express App (Routes)
const express = require("express");
const app     = express();
const path    = require("path");
const fileUpload = require('express-fileupload');
const mysql = require('mysql');
var connection;

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

app.get('/createDatabase', function(req, res) {
    connection = mysql.createConnection({
        host     : 'dursley.socs.uoguelph.ca',//database server
        user     : req.query.username,
        password : req.query.password,
        database : req.query.dbName
    });
    connection.connect(function(err) {
        if(!err) {
            connection.query("SHOW TABLES LIKE 'FILE'", function(err, result) {
                if(result.length === 0) {
                    connection.query("CREATE TABLE FILE (cal_id INT AUTO_INCREMENT, file_Name VARCHAR(60) NOT NULL, version INT NOT NULL, prod_id VARCHAR(256) NOT NULL, PRIMARY KEY(cal_id))",
                    function (err, rows, fields) {
                        if (err) console.log("Something went wrong creating FILE. "+err);
                    });
                } else {
                    console.log("Table FILE already existed");
                }
            });

            connection.query("SHOW TABLES LIKE 'EVENT'", function(err, result) {
                if(result.length === 0) {
                    connection.query("CREATE TABLE EVENT (event_id INT AUTO_INCREMENT, summary VARCHAR(1024), start_time DATETIME NOT NULL, location VARCHAR(60), organizer VARCHAR(256), cal_file INT NOT NULL, PRIMARY KEY(event_id), FOREIGN KEY(cal_file) REFERENCES FILE(cal_id) ON DELETE CASCADE)",
                    function (err, rows, fields) {
                        if (err) console.log("Something went wrong creating EVENT. "+err);
                    });
                } else {
                    console.log("Table EVENT already existed");
                }
            });

            connection.query("SHOW TABLES LIKE 'ALARM'", function(err, result) {
                if(result.length === 0) {
                    connection.query("CREATE TABLE ALARM (alarm_id INT AUTO_INCREMENT, action VARCHAR(256) NOT NULL, `trigger` VARCHAR(256) NOT NULL, event INT NOT NULL, PRIMARY KEY(alarm_id), FOREIGN KEY(event) REFERENCES EVENT(event_id) ON DELETE CASCADE)",
                    function (err, rows, fields) {
                        if (err) console.log("Something went wrong creating ALARM. "+err);
                    });
                } else {
                    console.log("Table ALARM already existed");
                }
            });
            res.send({
                msg:"Login successful"
            });

        } else {
            console.log(err);
            res.send({
                msg:"Error connecting to database, please retry"
            })
        }
    });
});

app.get('/logout', function(req, res) {
    connection.end();
    console.log("Closed connection to mySQL server");
    res.send({
        msg:"Logged out"
    })
});

app.get('/storeAllFiles', function(req, res) {
    //get files
    var fileArray = fs.readdirSync(__dirname+'/uploads');
    for(var i = 0; i < fileArray.length; i++) {
        let file = fileArray[i];
        //File data
        var jsonBuffer = new Buffer(500);
        libParser.getCalendarJSON(file, jsonBuffer);
        var calendarJSON = JSON.parse(ref.readCString(jsonBuffer));
        let insertFile = "INSERT INTO FILE (file_Name, version, prod_id) VALUES ('"+file+"', "+calendarJSON.version+", '"+calendarJSON.prodID+"')";
        //Check if file is already inserted
        connection.query("SELECT * FROM FILE WHERE `file_Name` = '"+file+"'", function(err, result) {
            if(!err) {
                if(result.length === 0) {
                    //Calendar hasn't been added yet
                    //console.log(insertFile);
                    connection.query(insertFile, function(err1, result1) {
                        if(err1) {
                            console.log("Something went wrong. "+err);
                        } else {
                            //Event data
                            let eventListBuffer = new Buffer(2000);
                            libParser.getEventListJSON(file, eventListBuffer);
                            var eventListJSON = JSON.parse(ref.readCString(eventListBuffer));
                            for(var j = 0; j < eventListJSON.length; j++) {
                                //YYYY-MM-DD HH:MM:SS
                                let formattedStart = eventListJSON[j].startDT.date.substring(0,4) + '-' + eventListJSON[j].startDT.date.substring(4,6) + '-' + eventListJSON[j].startDT.date.substring(6,8)+
                                " " + ("0" + eventListJSON[j].startDT.time.substring(0,2)).slice(-2) + ':' + ("0" + eventListJSON[j].startDT.time.substring(2,4)).slice(-2) + ':' +
                                ("0" + eventListJSON[j].startDT.time.substring(4,6)).slice(-2);

                                //Location and Organizer
                                let propertyListBuffer = new Buffer(2000);
                                let hack = "abcdefghij" + j.toString(10);
                                //the event number needs to be preceeded by ten characters
                                //adding them was faster than fixing it, hence 'hack'
                                libParser.getOptionalPropertiesJSON(file, hack, propertyListBuffer);
                                let propertyListJSON = JSON.parse(ref.readCString(propertyListBuffer));

                                let checkSummary;
                                if(eventListJSON[j].summary.length < 2) {
                                    checkSummary = "NULL";
                                } else {
                                    checkSummary = "'"+eventListJSON[j].summary+"'";
                                }
                                let checkLocation = "NULL";
                                for(let prop of propertyListJSON) {
                                    if(prop.name === "LOCATION") {
                                        checkLocation = "'"+prop.description+"'";
                                    }
                                }
                                let checkOrganizer = "NULL";
                                for(let prop of propertyListJSON) {
                                    if(prop.name === "ORGANIZER") {
                                        checkOrganizer = "'"+prop.description+"'";
                                    }
                                }

                                let insertEventString = "INSERT INTO EVENT (summary, start_time, location, organizer, cal_file) VALUES ("+checkSummary+
                                ", '"+formattedStart+"', "+checkLocation+", "+checkOrganizer+", "+result1.insertId+")";
                                //console.log(insertEventString);
                                connection.query(insertEventString, function(err2, result2) {
                                    if(err2) {
                                        console.log("Something went wrong. "+err2);
                                    } else {
                                        //Alarm data
                                        let alarmListBuffer = new Buffer(2000);
                                        libParser.getAlarmListJSON(file, j.toString(10), alarmListBuffer);
                                        let alarmListJSON = JSON.parse(ref.readCString(alarmListBuffer));
                                        //console.log(alarmListJSON);
                                        for(var k = 0; k < alarmListJSON.length; k++) {
                                            let alarmQuery = "INSERT INTO ALARM(action, `trigger`, event) VALUES('"+alarmListJSON[k].action+"', '"+alarmListJSON[k].trigger+"', "+result2.insertId+")";
                                            //console.log(alarmQuery);
                                            connection.query(alarmQuery, function(err3, rows3, fields3) {
                                                if(err) console.log("Something went wrong. "+err2);
                                            });
                                        }
                                    }
                                });
                            }
                        }
                    });
                } else {
                    //Calendar was added already
                    console.log("Stored "+file+" already");
                }
            } else {
                console.log("SQL problem. "+err);
            }
        });
    }//End of for loop
    res.send({
        msg:"Database updated"
    })
});

app.get('/clearAllData', function(req, res) {
    console.log("Deleting all database rows");
    connection.query("DELETE FROM ALARM", function(err, rows, fields) {
        if(err) console.log("Error deleting alarms. "+err);
    });
    connection.query("DELETE FROM EVENT", function(err, rows, fields) {
        if(err) console.log("Error deleting events. "+err);
    });
    connection.query("DELETE FROM FILE", function(err, rows, fields) {
        if(err) console.log("Error deleting files. "+err);
    });
});

app.get('/dbStatus', function(req, res) {
    connection.query("SELECT * FROM FILE", function(err, rows, fields) {
        if(!err) {
            var fileRows = rows.length;
            connection.query("SELECT * FROM EVENT", function(err1, rows1, fields1) {
                if(!err) {
                    var eventRows = rows1.length;
                    connection.query("SELECT * FROM ALARM", function(err2, rows2, fields2) {
                        if(!err){
                            var alarmRows = rows2.length;
                            res.send({
                                msg:"Database has "+fileRows+" files, "+eventRows+" events, and "+alarmRows+" alarms"
                            });
                        }
                    });
                }
            });
        }
    });
    /*res.send({*/
});

app.get('/dbQuery', function(req, res) {
    const data = req.query;
    switch(data.selection) {
        case '1':
            //all events sorted by start date
            connection.query("SELECT * FROM EVENT ORDER BY `start_time`", function(err, rows, fields) {
                if(!err) {
                    let output = [];
                    for(let row of rows) {
                        //console.log(row);
                        output.push(row);
                    }
                    console.log(output);
                    res.send(output);
                } else {
                    res.send({
                        msg:"Error processing query"
                    });
                }
            });
            break;
        case '2':
            //events from specific file
            const queryFileName = data.calendarName;
            connection.query("SELECT cal_id, file_Name FROM FILE", function(err, rows, fields) {
                if(!err) {
                    for(var i = 0; i < rows.length; i++) {
                        if(rows[i].file_Name === queryFileName) {
                            const matchingEvent = rows[i].cal_id;
                            connection.query("SELECT * FROM EVENT", function(err1, rows1, fields1) {
                                if(!err) {
                                    let output = [];
                                    for(var j = 0; j < rows1.length; j++) {
                                        if(rows1[j].cal_file === matchingEvent) {
                                            console.log(rows1[j]);
                                            output.push(rows1[j]);
                                        }
                                    }
                                    res.send(output);
                                } else {
                                    res.send({
                                        msg:"Error processing query"
                                    });
                                }
                            });
                        }
                    }
                } else {
                    res.send({
                        msg:"You must select a calendar"
                    });
                }
            });
            break;
        case '3':
            //events with conflicting start times
            connection.query("SELECT GROUP_CONCAT(start_time), GROUP_CONCAT(summary), GROUP_CONCAT(organizer), COUNT(*) c FROM EVENT GROUP BY start_time HAVING c > 1", function(err, rows, fields) {
                if(!err) {
                    let output = [];
                    for(let row of rows) {
                        output.push(row);
                    }
                    console.log(output);
                    res.send(output);
                } else {
                    res.send({
                        msg:"Error processing request"
                    });
                }
            });
            break;
        case '4':
            //events in the future
            connection.query("SELECT * FROM EVENT WHERE start_time > CURDATE() ORDER BY start_time", function(err, rows, fields) {
                if(!err) {
                    let output = [];
                    for(let row of rows) {
                        console.log(row);
                        output.push(row);
                    }
                    res.send(output);
                } else {
                    res.send({
                        msg:"Error processing request"
                    });
                }
            });
            break;
        case '5':
            //alarms with action=AUDIO
            connection.query("SELECT * FROM ALARM WHERE `action` = 'AUDIO'", function(err, rows, fields) {
                if(!err) {
                    let output = [];
                    for(let row of rows) {
                        console.log(row);
                        output.push(row);
                    }
                    res.send(output);
                } else {
                    res.send({
                        msg:"Error processing request"
                    });
                }
            });
            break;
        case '6':
            //events where locations is not null
            connection.query("SELECT * FROM EVENT WHERE `location` IS NOT NULL", function(err, rows, fields) {
                if(!err) {
                    let output = [];
                    for(let row of rows) {
                        console.log(row);
                        output.push(row);
                    }
                    res.send(output);
                } else {
                    res.send({
                        msg:"Error processing request"
                    });
                }
            });
            break;
        default:
            res.send({
                msg:"Error: must select a query option"
            });
            break;
    }
});

//Sample endpoint
app.get('/someendpoint', function(req , res){
  res.send({
    foo: "bar"
  });
});

app.listen(portNum);
console.log('Running app at localhost: ' + portNum);
