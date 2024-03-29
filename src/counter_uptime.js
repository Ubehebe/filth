uptime;

function get_put_counter()
{
  var maxlen = 11;
  var req = new XMLHttpRequest();
  req.open('GET', 'counter', true);
  req.setRequestHeader('Cache-Control', 'max-age = 0');

  req.onreadystatechange = function (dummy) {
    if (req.status == 200) {
      var inc = 1 * req.responseText + 1;
      var inc_str = inc + "";
      while (inc_str.length < maxlen)
	inc_str = "0" + inc_str;
      document.getElementById("counter").innerHTML = "→" + inc_str;
      var resp = new XMLHttpRequest();
      resp.open('PUT', 'counter', true);
      resp.setRequestHeader('Content-Length', inc.toString().length);
      resp.send(inc);
    }
  };
  req.send();
}

function get_uptime()
{
  var req = new XMLHttpRequest();
  req.open('GET', 'uptime', false);
  req.send();
  if (req.status == 200) {
    var d = new Date();
    uptime = Math.round(d.getTime()/1000) - req.responseText;
    window.setInterval(inc_uptime, 1000);
  }
}

function inc_uptime()
{
  ++uptime;
  var upstr = uptime + "";
  var maxlen = 11;
  while (upstr.length < maxlen)
    upstr = "0" + upstr;
  document.getElementById("uptime").innerHTML = "↑" + upstr;
}