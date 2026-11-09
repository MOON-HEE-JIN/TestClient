// Runs the actual wmain --login-only path against an isolated LoginServer.
const fs=require('node:fs'),path=require('node:path'),crypto=require('node:crypto'),net=require('node:net'),http=require('node:http'),{spawn}=require('node:child_process');
const root=path.resolve(__dirname,'..'),serverRoot=path.resolve(root,'../../LoginServer/LoginServer');
const out=path.join(__dirname,'output');fs.mkdirSync(out,{recursive:true});
const data=fs.mkdtempSync(path.join(out,'run-'));
const password=crypto.randomBytes(24).toString('hex'),game=crypto.randomBytes(32).toString('hex'),chat=crypto.randomBytes(32).toString('hex');
let server,base;const checks=[];
function check(ok,name){if(!ok)throw Error(name);checks.push(name);console.log('PASS '+name);}
const delay=ms=>new Promise(resolve=>setTimeout(resolve,ms));
async function freePort(){const listener=net.createServer();await new Promise(r=>listener.listen(0,'127.0.0.1',r));const port=listener.address().port;await new Promise(r=>listener.close(r));return port;}
async function start(){
 base=`http://127.0.0.1:${await freePort()}`;
 const config=JSON.parse(fs.readFileSync(path.join(serverRoot,'appsettings.json'),'utf8'));
 config.DevelopmentAuth={Enabled:true,UserId:'dev-http-client',Password:password};
 config.Internal={GameCredential:game,ChatCredential:chat};
 config.Operations={AdminName:'test-operator',AdminPassword:crypto.randomBytes(24).toString('hex'),HeartbeatTimeoutSeconds:30,AllowSimulation:false,DataDirectory:path.join(data,'operations')};
 fs.mkdirSync(config.Operations.DataDirectory,{recursive:true});
 fs.writeFileSync(path.join(config.Operations.DataDirectory,'settings.json'),JSON.stringify({settings:{GameServer:{maxConnect:2,version:1},ChatServer:{maxConnect:null,version:0}},audit:[]}));
 fs.writeFileSync(path.join(data,'appsettings.json'),JSON.stringify(config));
 const env={...process.env,DOTNET_ENVIRONMENT:'Development',ASPNETCORE_ENVIRONMENT:'Development'};
 for(const key of Object.keys(env))if(/^(DevelopmentAuth|Internal|Operations|Sessions|Policies)__/i.test(key))delete env[key];
 const fd=fs.openSync(path.join(data,'server.log'),'a');
 server=spawn('dotnet',[path.join(serverRoot,'bin/Debug/net8.0/LoginServer.dll'),'--urls',base,'--contentRoot',data,'--webroot',path.join(serverRoot,'wwwroot')],{cwd:serverRoot,env,windowsHide:true,stdio:['ignore',fd,fd]});fs.closeSync(fd);
 for(let i=0;i<100;i++){if(server.exitCode!==null)throw Error('LoginServer startup failed');try{if((await fetch(base+'/health')).ok)return;}catch{}await delay(100);}throw Error('LoginServer startup timeout');
}
async function post(route,body,internal=false){return fetch(base+route,{method:'POST',headers:{'Content-Type':'application/json',...(internal?{'X-Server-Id':'GameServer','X-Server-Credential':game}:{})},body:JSON.stringify(body)});}
async function run(exe,overrides={}){
 const env={...process.env};for(const key of Object.keys(env))if(/^LOGIN_/i.test(key))delete env[key];
 Object.assign(env,{LOGIN_SERVER_URL:base+'/',LOGIN_ACCOUNT:'dev-http-client',LOGIN_PASSWORD:password},overrides);
 return new Promise((resolve,reject)=>{
  const child=spawn(exe,['--login-only'],{cwd:path.dirname(exe),env,windowsHide:true});let output='';
  const timer=setTimeout(()=>{child.kill();reject(Error('TestClient login timed out'));},12000);
  child.stdout.on('data',b=>output+=b.toString('utf8'));child.stderr.on('data',b=>output+=b.toString('utf8'));
  child.once('error',e=>{clearTimeout(timer);reject(e);});child.once('exit',code=>{clearTimeout(timer);resolve({code,output});});
 });
}
(async()=>{try{
 await start();
 for(const mode of ['Debug','Release']){
  const exe=path.resolve(root,`../x64/${mode}/TestClient.exe`);
  let result=await run(exe,{LOGIN_PASSWORD:''});check(result.code===2&&result.output.includes('Set LOGIN_ACCOUNT'),mode+' wmain rejects missing credentials before launching UI');
  result=await run(exe,{LOGIN_PASSWORD:'wrong-password'});check(result.code===1&&result.output.includes('HTTP 401')&&result.output.includes('INVALID_CREDENTIALS'),mode+' handles invalid login credentials');
  result=await run(exe,{LOGIN_ACCOUNT:'한글ID'});check(result.code===1&&result.output.includes('HTTP 401')&&result.output.includes('INVALID_CREDENTIALS'),mode+' sends Unicode environment credentials as valid UTF-8 JSON');
  const registration=await(await post('/internal/operations/register',{instanceId:crypto.randomUUID()},true)).json();
  await post('/internal/operations/disconnect',{lease:registration.lease},true);
  result=await run(exe);check(result.code===1&&result.output.includes('SERVER_UNAVAILABLE'),mode+' handles unavailable GameServer');
  check((await post('/api/serverconnection/register',{serverId:'GameServer',type:0,address:'127.0.0.1',port:7799})).status===200,mode+' isolated server registration ready');
  result=await run(exe);check(result.code===1&&result.output.includes('SERVER_STATUS_UNAVAILABLE'),mode+' rejects missing capacity report');
  check((await post('/api/serverconnection/heartbeat',{serverId:'GameServer',type:0,connect:0})).status===200,mode+' isolated capacity report ready');
  result=await run(exe);check(result.code===0&&result.output.includes('HTTP 200')&&result.output.includes('token received'),mode+' actual wmain exchanges JSON login and receives token');
  check(!result.output.includes(password)&&!result.output.includes('"token":'),mode+' keeps credentials and raw session token out of console output');
  await post('/api/serverconnection/heartbeat',{serverId:'GameServer',type:0,connect:2});
  result=await run(exe);check(result.code===1&&result.output.includes('HTTP 409')&&result.output.includes('SERVER_FULL'),mode+' login-only reports the full-server failure');
  result=await run(exe,{LOGIN_SERVER_URL:`http://127.0.0.1:${await freePort()}`});check(result.code===1&&result.output.includes('HTTP 0'),mode+' distinguishes network failure from HTTP response');
  const malformed=http.createServer((req,res)=>{req.resume();res.writeHead(200,{'Content-Type':'application/json'});res.end('{broken-json');});
  await new Promise(r=>malformed.listen(0,'127.0.0.1',r));
  try{result=await run(exe,{LOGIN_SERVER_URL:`http://127.0.0.1:${malformed.address().port}`});check(result.code===1&&result.output.includes('Invalid LoginServer JSON response'),mode+' rejects malformed success responses');}finally{await new Promise(r=>malformed.close(r));}
 }
 fs.writeFileSync(path.join(out,'results.json'),JSON.stringify({passed:checks.length,checks},null,2));console.log('PASS '+checks.length+' checks');
}catch(error){console.error(error);process.exitCode=1;}finally{if(server&&server.exitCode===null){const stopped=new Promise(r=>server.once('exit',r));server.kill();await stopped;}}})();
