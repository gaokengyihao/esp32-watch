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
let tr0=0, prev0=bit(ch[0].buf,0), prev1=0, rise1=0, first=-1;
for(let s=1;s<N;s++){
 const a=bit(ch[0].buf,s), b=bit(ch[1].buf,s);
 if(a!==prev0){tr0++;prev0=a;}
 if(b&&!prev1){rise1++; if(first<0) first=s;}
 prev1=b;
}
console.log('CH0 transitions:',tr0,' CH1 rising edges:',rise1,' ratio:',(tr0/rise1).toFixed(2));
console.log('CH0 at first CH1 rising:',bit(ch[0].buf,first)?'H':'L');
