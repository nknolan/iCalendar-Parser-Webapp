// Put all onload AJAX calls here, and event listeners
$(document).ready(function() {
    // On page-load AJAX Example
    $.ajax({
        type: 'get',            //Request type
        dataType: 'json',       //Data type - we will use JSON for almost everything
        url: '/someendpoint',   //The server endpoint we are connecting to
        success: function (data) {
            /*  Do something with returned object
                Note that what we get is an object, not a string,
                so we do not need to parse it on the server.
                JavaScript really does handle JSONs seamlessly
            */
            $('#blab').html("On page load, Received string '"+JSON.stringify(data)+"' from server");
            //We write the object to the console to show that the request was successful
            console.log(data);

        },
        fail: function(error) {
            // Non-200 return, do something with error
            console.log(error);
        }
    });

    $.ajax({
        type: 'get',            //Request type
        dataType: 'json',       //Data type - we will use JSON for almost everything
        url: '/fileList',   //The server endpoint we are connecting to
        success: function (data) {
            console.log(data);
            let calendarDropdown = $('#calendar-panel-dropdown');
            calendarDropdown.empty();
            calendarDropdown.append('<option selected="true" disabled>Choose a Calendar</option>');
            calendarDropdown.prop('selectedIndex', 0);
            let eventDropdown = $('#event-panel-dropdown');
            eventDropdown.empty();
            eventDropdown.append('<option selected="true" disabled>Choose a Calendar</option>');
            eventDropdown.prop('selectedIndex', 0);
            let queryDropdown = $('#query-dropdown');
            queryDropdown.empty();
            queryDropdown.append('<option selected="true" disabled>Choose a Calendar</option>');
            queryDropdown.prop('selectedIndex', 0);
            $.each(data, function(key, value) {
                if(value.split('.').pop() == "ics") {
                    $('#calendar-panel-dropdown').append($('<option></option>').html(value));
                    $('#event-panel-dropdown').append($('<option></option>').html(value));
                    $('#query-dropdown').append($('<option></option>').html(value));
                }
            });
        },
        error: function(error) {
            // Non-200 return, do something with error
            console.log(error);
        }
    });

    //Calendar panel
    $.ajax({
        type: 'get',
        dataType: 'html',
        url:'/fileTable',
        success: function(data) {
            document.getElementById('file-table').innerHTML = data;
        },
        error: function(error) {
            console.log(error);
        }
    });

    //File panel
    $('#select-box').change(function() {
        var selection = "";
        $("#select-box option:selected").each(function() {
            selection = $(this).text();
            $.ajax({
                type:'get',
                dataType:'json',
                url:'/eventTable',
                data: {
                    calendarName:selection
                },
                success: function(data) {
                    var eventTable = '<table><tr><th>Event No</th><th>Start date</th><th>Start time</th>'+
                    '<th>Summary</th><th>Props</th><th>Alarms</th><th>Show Alarms</th><th>Extract Optional Properties</th></tr>';
                    for(var i = 0; i < data.length; i++) {
                        if(data[i].hasOwnProperty("error")) {
                            eventTable+='<tr><td></td><td></td><td></td><td></td><td></td><td></td><td></td><td></td></tr>';
                        } else {
                            var dateString = data[i].startDT.date.substring(0,4) + '/' + data[i].startDT.date.substring(4,6) + '/' + data[i].startDT.date.substring(6,8);
                            var timeString = ("0" + data[i].startDT.time.substring(0,2)).slice(-2) + ':' +
                                ("0" + data[i].startDT.time.substring(2,4)).slice(-2) + ':' +
                                ("0" + data[i].startDT.time.substring(4,6)).slice(-2) + (data[i].startDT.isUTC ? '(UTC)' : '');
                            eventTable+='<tr><td>'+(i+1)+'</td><td>'+dateString+'</td><td>'+timeString+'</td><td>'+data[i].summary+'</td><td>'+
                            data[i].numProps+'</td><td>'+data[i].numAlarms+'</td><td><input type="button" class="showAlarmButton" id="alarms'+i+'" value="Alarms"></td>'+
                            '<td><input type="button" class="showPropertyButton" id="properties'+i+'" value="Properties"></td></tr>';
                        }
                    }
                    eventTable+='</table><br>';
                    document.getElementById('calendar-table').innerHTML = "" + eventTable;
                },
                error: function(error) {
                    console.log(error);
                }
            });
        });
    });

    //Submit new event to existing calendar
    $('#upload-event').click(function() {
        let createDate = document.getElementById('creation-date').value;
        let createTime = document.getElementById('creation-time').value;
        let createUTC = document.getElementById('creation-UTC').checked;
        let startDate = document.getElementById('start-date').value;
        let startTime = document.getElementById('start-time').value;
        let startUTC = document.getElementById('start-UTC').checked;
        let uidVal = document.getElementById('uid').value;
        let summaryVal = document.getElementById('event-summary').value;//OPTIONAL
        let valid = 0;
        if(createDate.length != 8 || isNaN(createDate)) {
            valid++;
            addStatus("Creation Date is invalid");
        }
        if(createTime.length != 6 || isNaN(createTime)) {
            valid++;
            addStatus("Creation Time is invalid");
        }
        if(startDate.length != 8 || isNaN(startDate)) {
            valid++;
            addStatus("Start Date is invalid");
        }
        if(startTime.length != 6 || isNaN(startTime)) {
            valid++;
            addStatus("Creation Time is invalid");
        }
        if(uidVal.length == 0) {
            valid++;
            addStatus("UID is invalid");
        }
        if(valid === 0) {
            $.ajax({
                type:'get',
                url:'/addEvent',
                dataType:'json',
                data: {
                    calendarName:document.getElementById('event-panel-dropdown').value,
                    creationDate:createDate,
                    creationTime:createTime,
                    creationUTC:createUTC,
                    startDate:startDate,
                    startTime:startTime,
                    startUTC:startUTC,
                    uid:uidVal,
                    summary:summaryVal
                },
                success: function(data) {
                    addStatus(JSON.stringify(data));
                    console.log(data);
                },
                error: function(error) {
                    addStatus(data);
                    console.log(error);
                }
            })
        }
    });

    $('#create-calendar').click(function() {
        //CALENDAR
        var newFile = document.getElementById('create-fileName').value;
        var newProdID = document.getElementById('create-prodid').value;
        var newVersionNum = document.getElementById('create-versionNum').value;
        //EVENT
        var newCreateDate = document.getElementById('create-creation-date').value;
        var newCreateTime = document.getElementById('create-creation-time').value;
        var newCreateUTC = document.getElementById('create-creation-UTC').checked;
        var newStartDate = document.getElementById('create-start-date').value;
        var newStartTime = document.getElementById('create-start-time').value;
        var newStartUTC = document.getElementById('create-start-UTC').checked;
        var newUidVal = document.getElementById('create-uid').value;
        /*<label for="entryBox">Event ID</label>
        <input type="text" class="form-control" id="create-uid" placeholder="Event ID">*/
        var newSummaryVal = document.getElementById('create-summary').value;//OPTIONAL
        let valid = 0;
        if(newFile.split('.').pop() != "ics") {
            valid++;
            addStatus("Invalid file name");
        }
        if(isNaN(newVersionNum)) {
            valid++;
            addStatus("Invalid version number");
        }
        if(newProdID.length < 1) {
            valid++;
            addStatus("Invalid calendar ID");
        }
        if(newCreateDate.length != 8 || isNaN(newCreateDate)) {
            valid++;
            addStatus("Creation Date is invalid");
        }
        if(newCreateTime.length != 6 || isNaN(newCreateTime)) {
            valid++;
            addStatus("Creation Time is invalid");
        }
        if(newStartDate.length != 8 || isNaN(newStartDate)) {
            valid++;
            addStatus("Start Date is invalid");
        }
        if(newStartTime.length != 6 || isNaN(newStartTime)) {
            valid++;
            addStatus("Creation Time is invalid");
        }
        if(newUidVal.length == 0) {
            valid++;
            addStatus("UID is invalid");
        }
        var toUpload = {
            fileName:newFile,
            version:newVersionNum,
            prodID:newProdID,
            creationDate:newCreateDate,
            creationTime:newCreateTime.slice(),
            creationUTC:newCreateUTC,
            startDate:newStartDate,
            startTime:newStartTime,
            startUTC:newStartUTC,
            uid:newUidVal,
            summary:newSummaryVal
        }
        $.ajax({
            type:'get',
            url:'/createCalendar',
            dataType:'json',
            data: toUpload,
            success: function(data) {
                addStatus(data);
                console.log(data);
            },
            error: function(error) {
                console.log(error);
            }
        });
    });

    //Database login form
    $('#db-login').click(function() {
        var username = document.getElementById('username').value;
        var password = document.getElementById('password').value;
        var dbName = document.getElementById('db-name').value;
        let valid = 0;
        if(username.length === 0) {
            valid++;
            addStatus("Must enter username");
        }
        if(password.length === 0) {
            valid++;
            addStatus("Must enter password");
        }
        if(dbName.length === 0) {
            valid++;
            addStatus("Must enter database name");
        }
        if(valid === 0) {
            $.ajax({
                type:'get',
                url:'/createDatabase',
                dataType:'json',
                data:{
                    username:username,
                    password:password,
                    dbName:dbName
                },
                success: function(data) {
                    addStatus(data.msg);
                    showQueryPanel();
                },
                error: function(error) {
                    console.log(error);
                }
            });
        }
    });
    $('#logout').click(function() {
        $.ajax({
            type:'get',
            url:'/logout',
            datatype:'json',
            success: function(data) {
                hideQueryPanel();
                addStatus(data.msg);
            },
            error: function(error) {
                console.log(error);
            }
        });
    });

    $('#store-all-files').click(function() {
        //Needs to bounce if database is unconnected or no files
        if(true) {
            $.ajax({
                type:'get',
                url:'/storeAllFiles',
                dataType:'json',
                success: function(data) {
                    console.log(data);
                    addStatus(data.msg);
                },
                error: function(error) {
                    console.log(error);
                }
            });
        } else {
            //bounce
        }
    });

    $('#clear-all-data').click(function() {
        if(true) {
            $.ajax({
                type:'get',
                url:'/clearAllData',
                dataType:'json',
                success: function(data) {
                    console.log(data);
                },
                error: function(error) {
                    console.log(error);
                }
            });
        } else {
            //bounce
        }
    });

    $('#display-db-status').click(function() {
        if(true) {
            $.ajax({
                type:'get',
                url:'/dbStatus',
                dataType:'json',
                success: function(data) {
                    addStatus(data.msg);
                    console.log(data);
                },
                error: function(error) {
                    console.log(error);
                }
            });
        } else {
            //bounce
        }
    });

    $('#submit-query').click(function() {
        $.ajax({
            type:'get',
            url:'/dbQuery',
            dataType:'json',
            data: {
                selection:$(".query-button:checked").val(),
                calendarName:document.getElementById('query-dropdown').value
            },
            success: function(data) {
                if(data.hasOwnProperty('msg')) {
                    addStatus(data.msg);
                } else {
                    switch($(".query-button:checked").val()) {
                        case '1':
                            addStatus("All events by start date:");
                            for(var i = 0; i < data.length; i++) {
                                addStatus("EVENT");
                                if(data[i].summary) {
                                    addStatus("Summary: "+data[i].summary);
                                }
                                addStatus("Start: "+data[i].start_time);
                                if(data[i].location) {
                                    addStatus("Location: "+data[i].location);
                                }
                                if(data[i].organizer) {
                                    addStatus("Organizer: "+data[i].organizer);
                                }
                            }
                            break;
                        case '2':
                            addStatus("All events from chosen file:");
                            for(var i = 0; i < data.length; i++) {
                                addStatus("EVENT");
                                if(data[i].summary) {
                                    addStatus("Summary: "+data[i].summary);
                                }
                                addStatus("Start: "+data[i].start_time);
                            }
                            break;
                        case '3':
                            addStatus("All events with conflicting start datetimes:");
                            for(var i = 0; i < data.length; i++) {
                                addStatus("Conflict date: "+data[i]["GROUP_CONCAT(start_time)"]);
                                addStatus("Summaries: "+data[i]["GROUP_CONCAT(summary)"]);
                                addStatus("Organizers: "+data[i]["GROUP_CONCAT(organizer)"]);
                            }
                            break;
                        case '4':
                            addStatus("All future events:");
                            for(var i = 0; i < data.length; i++) {
                                addStatus("EVENT");
                                if(data[i].summary) {
                                    addStatus("Summary: "+data[i].summary);
                                }
                                addStatus("Start: "+data[i].start_time);
                                if(data[i].location) {
                                    addStatus("Location: "+data[i].location);
                                }
                                if(data[i].organizer) {
                                    addStatus("Organizer: "+data[i].organizer);
                                }
                            }
                            break;
                        case '5':
                            addStatus("All alarms with an 'AUDIO' action:");
                            for(var i = 0; i < data.length; i++) {
                                addStatus("ALARM: trigger = "+data[i].trigger+", action = "+data[i].action+" ");
                            }
                            break;
                        case '6':
                            addStatus("All events with a defined location:");
                            for(var i = 0; i < data.length; i++) {
                                addStatus("EVENT");
                                if(data[i].summary) {
                                    addStatus("Summary: "+data[i].summary);
                                }
                                addStatus("Start: "+data[i].start_time);
                                if(data[i].location) {
                                    addStatus("Location: "+data[i].location);
                                }
                                if(data[i].organizer) {
                                    addStatus("Organizer: "+data[i].organizer);
                                }
                            }
                            break;
                        default:
                            addStatus("Weird error. Try again.");
                            break;
                    }
                }
                console.log(data);
            },
            error: function(error) {
                console.log(error);
            }
        });
    });

    // Event listener form replacement example, building a Single-Page-App, no redirects if possible
    $('#someform').submit(function(e){
        $('#blah').html("Callback from the form");
        e.preventDefault();
        //Pass data to the Ajax call, so it gets passed to the
        $.ajax({});
    });

});

//Add event listeners to dynamically loaded DOM elements

//Send alarmlist JSON to status panel
$(document).on('click', '.showAlarmButton', function() {
    var selection = $("#select-box :selected").text();
    var id = this.id;
    $.ajax({
        type:'get',
        dataType: 'json',
        url:'/alarmJSON',
        data: {
            info:selection + '_' + id
        },
        success: function(data) {
            console.log(data);
            if(JSON.stringify(data).length < 4) {
                addStatus("No alarms");
            } else {
                for(var i = 0; i < data.length; i++) {
                    addStatus("Alarm: action = '"+data[i].action+"', trigger = '"+data[i].trigger+"', numProps = '"+data[i].numProps+"'");
                }
            }
        },
        failure: function(error) {
            console.log(error);
            addStatus("Server error getting alarms");
        }
    });
});

//Send propertylist JSON to status panel
$(document).on('click', '.showPropertyButton', function() {
    var selection = $("#select-box :selected").text();
    var id = this.id;
    $.ajax({
        type:'get',
        dataType: 'JSON',
        url:'/propertyJSON',
        data: {
            info:selection + '_' + id
        },
        success: function(data) {
            console.log(data);
            if(JSON.stringify(data).length < 4) {
                addStatus("No optional properties");
            } else {
                for(var i = 0; i < data.length; i++) {
                    addStatus("Property: name = '"+data[i].name+"', description = '"+data[i].description+"'");
                }
            }
        },
        failure: function(error) {
            console.log(error);
            addStatus("Server error getting properties");
        }
    });
});

function showAddEvent() {
    var menu = document.getElementById('event-form');
    var choice = document.getElementById('event-panel-dropdown');
    if(choice != "") {
        menu.style.display = "block";
    } else {
        menu.style.display = "none";
    }
}

function showQueryPanel() {
    var panel = document.getElementById('query-panel');
    panel.style.display = "block";
}

function hideQueryPanel() {
    var panel = document.getElementById('query-panel');
    panel.style.display = "none";
}

function addStatus(msg) {
    document.getElementById('status').innerHTML += (msg + '<br>');
}

function clearStatus() {
    document.getElementById('status').innerHTML = ("");
}

function checkExtension() {
    var file = document.getElementById('file-name-holder').value;
    if(file.split('.').pop() != "ics") {
        alert("Uploaded an invalid file");
        addStatus("Uploaded an invalid file");
    }
}
