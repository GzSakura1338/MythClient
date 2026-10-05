using System;
using System.IO;
using System.IO.Compression;
using System.Text;
using System.Linq;
using System.Collections.Generic;
using System.Text.RegularExpressions;
public sealed class LunarClassAudit {
    public static void GenerateProbes(string sourceRoot,string output){
        var files=new List<string>{Path.Combine(sourceRoot,"compat/mappings/minecraft263.inc")};
        files.AddRange(Directory.EnumerateFiles(Path.Combine(sourceRoot,"compat/minecraft263"),"*.cpp"));
        files.AddRange(Directory.EnumerateFiles(Path.Combine(sourceRoot,"compat/lunar12111"),"*.cpp"));
        var lines=new HashSet<string>();
        foreach(var file in files)foreach(Match m in Regex.Matches(File.ReadAllText(file),"\"((?:net[./]minecraft|com[./]mojang|java[./]|org[./])[^\"]*)\"\\s*,\\s*\"([^\"]*)\"\\s*,\\s*\"([^\"]*)\"")){
            string owner=m.Groups[1].Value,name=m.Groups[2].Value,desc=m.Groups[3].Value;
            if(!Regex.IsMatch(name,@"^[\w$<>]+$")||!Regex.IsMatch(desc,@"^(?:\(|L|\[|[ZBSCIJFD]$)"))continue;
            lines.Add("print(\"extra:"+owner+"."+name+desc+"\", \""+owner+"\", \""+name+"\", \""+desc+"\");");
        }
        var ec=File.ReadAllText(Path.Combine(sourceRoot,"compat/mappings/easecation.inc"));
        ec=ec.Substring(0,ec.IndexOf("#elif defined(NOVA_EASECATION_TYPE)",StringComparison.Ordinal));
        foreach(Match m in Regex.Matches(ec,"NOVA_(?:TEAM|BT)\\([^,]+,\\s*\"([^\"]+)\",\\s*\"([^\"]+)\",\\s*\"([^\"]+)\"\\)"))
            lines.Add("print(\"override:"+m.Groups[1].Value+"."+m.Groups[2].Value+"\", \""+m.Groups[1].Value+"\", \""+m.Groups[2].Value+"\", \""+m.Groups[3].Value+"\");");
        File.WriteAllLines(output,lines.OrderBy(s=>s,StringComparer.Ordinal));
        string catalog=File.ReadAllText(Path.Combine(sourceRoot,"compat/mappings/minecraft263.inc"));
        int start=catalog.IndexOf("#elif defined(NOVA_MC263_BINDINGS)",StringComparison.Ordinal);
        int end=catalog.IndexOf("#elif",start+6,StringComparison.Ordinal);
        string hooks=catalog.Substring(start,end-start)+File.ReadAllText(Path.Combine(sourceRoot,"compat/minecraft263/Minecraft263Hooks.cpp"));
        var native=new List<string>();
        foreach(Match m in Regex.Matches(hooks,"\\{\\s*\"(\\w+)\"\\s*,\\s*\"(\\([^\"]+)\"\\s*,\\s*reinterpret_cast<void\\*>"))
            native.Add("expected.emplace(\""+m.Groups[1].Value+"\",nova::compat::lunar12111::Descriptor(\""+m.Groups[2].Value+"\"));");
        if(native.Count<30)throw new InvalidDataException("Missing native hook bindings");
        File.WriteAllLines(Path.Combine(Path.GetDirectoryName(output),"native-hooks.inc"),native);
    }
    sealed class Class { public string[] Parents; public HashSet<string> Members=new HashSet<string>(); }
    static int U2(BinaryReader r){return (r.ReadByte()<<8)|r.ReadByte();}
    static uint U4(BinaryReader r){return ((uint)U2(r)<<16)|(uint)U2(r);}
    static void Attributes(BinaryReader r){int n=U2(r);for(int i=0;i<n;i++){U2(r);uint length=U4(r);r.BaseStream.Position+=length;}}
    static Class Read(byte[] bytes){
        using(var r=new BinaryReader(new MemoryStream(bytes))){
            if(U4(r)!=0xcafebabe)throw new InvalidDataException("class magic");
            U2(r);U2(r);int n=U2(r);string[] utf=new string[n];int[] refs=new int[n];
            for(int i=1;i<n;i++)switch(r.ReadByte()){
                case 1:utf[i]=Encoding.UTF8.GetString(r.ReadBytes(U2(r)));break;
                case 7:refs[i]=U2(r);break;
                case 8:case 16:case 19:case 20:U2(r);break;
                case 3:case 4:case 9:case 10:case 11:case 12:case 17:case 18:U4(r);break;
                case 5:case 6:U4(r);U4(r);++i;break;
                case 15:r.ReadByte();U2(r);break;
                default:throw new InvalidDataException("constant pool");
            }
            U2(r);U2(r);int parent=U2(r);var parents=new List<string>();if(parent!=0)parents.Add(utf[refs[parent]]);
            int ni=U2(r);for(int i=0;i<ni;i++)parents.Add(utf[refs[U2(r)]]);
            var c=new Class{Parents=parents.ToArray()};
            for(int kind=0;kind<2;kind++){int count=U2(r);for(int i=0;i<count;i++){U2(r);string name=utf[U2(r)],desc=utf[U2(r)];c.Members.Add(name+desc);Attributes(r);}}
            return c;
        }
    }
    public static int Check(string bake,string bindings,string sourceRoot,string exclusions){
        var used=new HashSet<string>();
        foreach(var f in Directory.EnumerateFiles(sourceRoot,"*",SearchOption.AllDirectories).Where(f=>f.EndsWith(".cpp")||f.EndsWith(".h")))
            foreach(Match m in Regex.Matches(File.ReadAllText(f),@"\b\w+\b"))used.Add(m.Value);
        var excluded=new HashSet<string>(File.Exists(exclusions)?File.ReadAllLines(exclusions).Where(l=>!l.StartsWith("#")):new string[0]);
        using(var zip=ZipFile.OpenRead(bake)){
            var cache=new Dictionary<string,Class>();
            Func<string,string,string, bool> lookup=null;
            lookup=(owner,name,desc)=>{
                if(owner.StartsWith("java/")||owner.StartsWith("org/")||owner.StartsWith("io/netty/"))return false;
                if(!cache.TryGetValue(owner,out var c)){
                    var entry=zip.GetEntry(owner.Replace('/','.'))??zip.GetEntry(owner+".class");
                    if(entry==null)return false;
                    using(var ms=new MemoryStream()){using(var s=entry.Open())s.CopyTo(ms);c=Read(ms.ToArray());}cache.Add(owner,c);
                }
                if(name.Length==0||c.Members.Contains(name+desc))return true;
                return name!="<init>"&&c.Parents.Where(p=>p!="java/lang/Object").Any(p=>lookup(p,name,desc));
            };
            int failures=0,count=0;
            foreach(var line in File.ReadLines(bindings)){
                var p=line.Split('\t');if(p.Length!=7)continue;
                if(p[4].StartsWith("java.")||p[4].StartsWith("org.")||p[4].StartsWith("io.netty."))continue;
                var id=p[0].StartsWith("class:")?p[0].Substring(6):p[0];
                if(excluded.Contains(p[0])||(!p[0].StartsWith("ec_")&&!p[0].StartsWith("override:")&&!p[0].StartsWith("extra:")&&!used.Contains(id)))continue;
                ++count;if(!lookup(p[4].Replace('.','/'),p[5],p[6])){Console.WriteLine(line);++failures;}
            }
            Console.WriteLine("JNI catalog: "+count+" bindings, "+failures+" failures");return failures;
        }
    }
}
