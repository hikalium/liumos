pub const DEFAULT_PAGE: &str = r#"<html>
<head>
  <style>
    .leaf {
      background-color: green;
      height: 5;
      width: 5;
    }
    #leaf1 {
      margin-top: 50;
      margin-left: 275;
    }
    #leaf2 {
      margin-left: 270;
    }
    #leaf3 {
      margin-left: 265;
    }
    #id2 {
      background-color: orange;
      height: 20;
      width: 30;
      margin-left: 250;
    }
    #id3 {
      background-color: lightgray;
      height: 30;
      width: 80;
      margin-top: 3;
      margin-left: 225;
    }
    #id4 {
      background-color: lightgray;
      height: 30;
      width: 100;
      margin-top: 3;
      margin-left: 215;
    }
  </style>
</head>
<body>
  <div class=leaf id=leaf1></div>
  <div class=leaf id=leaf2></div>
  <div class=leaf id=leaf3></div>
  <div id=id2></div>
  <div id=id3></div>
  <div id=id4></div>
</body>
</html>"#;
