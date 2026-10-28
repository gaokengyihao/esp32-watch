const fs=require('fs'),path=require('path');
const base=process.argv[2],FS=5000000,BLK=1048576;
function loadCh(ch){
 const dir=path.join(base,String(ch));
 const ini=fs.readFileSync(path.join(dir,'channel.ini'),'utf8').split(/\r?\n/).map(s=>s.replace(/^\uFEFF/,'').trim());
 const total=parseInt(ini[2],10)||0; let bitmap=0n;
 for(const l of ini){const m=l.match(/^(\d+),(\d+)$/); if(m){bitmap=BigInt(m[1]);break;}}
 const present=new Set(fs.readdirSync(dir).filter(f=>/^0-\d+\.bin$/.test(f)).map(f=>parseInt(f.match(/0-(\d+)\.bin/)[1],10)));
 let maxIdx=present.size?Math.max(...present):-1;
 for(let b=0;b<64;b++) if((bitmap>>BigInt(b))&1n) maxIdx=Math.max(maxIdx,b);
 if(total>0) maxIdx=Math.max(maxIdx,Math.ceil(total/(BLK*8))-1);
 const zero=Buffer.alloc(BLK),parts=[];
 for(let i=0;i<=maxIdx;i++) parts.push(present.has(i)?fs.readFileSync(path.join(dir,`0-${i}.bin`)):zero);
 let buf=Buffer.concat(parts); if(total>0){const bytes=Math.ceil(total/8); if(buf.length>bytes) buf=buf.subarray(0,bytes);}
 return {buf,total:total||buf.length*8};
}
const ch=[0,1].map(loadCh);
const bit=(b,s)=>(b[s>>3]>>(s&7))&1;
const N=Math.min(...ch.map(c=>c.total));
let prev=0,first=-1;
for(let s=0;s<N;s++){const c=bit(ch[1].buf,s); if(c&&!prev){first=s;break;} prev=c;}
console.log('first CLK edge at', (first/FS*1000).toFixed(3), 'ms');
const start=Math.max(0,first-100*5000), end=Math.min(N,first+300*5000);
let cs=bit(ch[0].buf,start), last=start;
for(let s=start+1;s<end;s++){
 const v=bit(ch[0].buf,s);
 if(v!==cs){ console.log('CS', cs?'H':'L','->',v?'H':'L','at',((s-first)/FS*1000).toFixed(3),'ms; prev dur',((s-last)/FS*1000).toFixed(3),'ms'); cs=v; last=s; }
}
console.log('CS at first CLK edge:', bit(ch[0].buf,first)?'H':'L');
