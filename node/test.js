var stash = require('./build/Release/stash');

var template = new stash.Template(10);
console.log( template.render() ); // 11
console.log( template.render() ); // 12
console.log( template.render() ); // 13
