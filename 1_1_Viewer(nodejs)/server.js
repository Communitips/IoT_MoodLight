var express = require('express')
var bodyParser = require('body-parser');
var fs = require('fs')

var app = express()
var mysql = require('mysql')
var client = mysql.createConnection({
  user: 'root',
  password: 'tips20!7',
  database: 'SensorNode',
  dateStrings: true
})


app.use(express.static('public'))
app.use(bodyParser.urlencoded({extended:false}));

app.get('/Temperature', function(req, res){
		var text = fs.readFile('./public/chart.html', 'utf-8', function(error, data){
		    if(error){
	    		console.log(error)
  			}else{
    			res.send(data)
    		}
		})	
})

app.get('/Temperature/ajax', function(req, res){
	
  client.query('SELECT * FROM Temperature', function(error, result, fields){
    if(error){
	console.log(error)
    }else{
		res.send(result)
    }    
  })

})
app.listen(25601, function(){
  console.log('25601 server running')
})
