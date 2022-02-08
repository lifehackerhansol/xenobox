#!/usr/bin/env ruby
require 'net/http'
require 'zipruby'

SAX=2
case SAX
	when 0
		require 'rexml/document'
		require 'rexml/parsers/streamparser' 
		require 'rexml/streamlistener'
	when 1
		require 'rexml/document'
		require 'rexml/parsers/sax2parser'
		require 'rexml/sax2listener'
	when 2
		begin
			require 'libxml'
		rescue LoadError
			puts 'Falling back to REXML. Please: gem install libxml-ruby'
			SAX=0
			require 'rexml/document'
			require 'rexml/parsers/streamparser' 
			require 'rexml/streamlistener'
		end
end

cont=''
gamelist_name=ARGV[0]?ARGV[0]:'GameList.txt';
STDERR.puts "Phase1: Read #{gamelist_name}";
open(gamelist_name,'rb'){|f|
	cont=f.read
}

body=''
STDERR.puts "Phase2: Download ADVANsCEne_NDScrc.zip";
Net::HTTP.start('www.advanscene.com',80){|http|
	http.request_get("/offline/datas/ADVANsCEne_NDScrc.zip",{
		'Accept'=>'*/*'
	}){|response|
		response.read_body{|str|
			body<<str
			STDERR.printf("%d / %d (%.1f%%)\r",body.size,response.content_length,body.size*100.0/response.content_length)
		}
	}
}

xml=''
STDERR.puts "Phase3: Decompress ADVANsCEne_NDScrc.zip";
Zip::Archive.open_buffer(body){|zip|
	zip.each{|f|
		if f.name=='ADVANsCEne_NDScrc.xml' then xml=f.read;break end
	}
}

class GameListListener
	case SAX
		when 0 then include REXML::StreamListener
		when 1 then include REXML::SAX2Listener
		when 2 then include LibXML::XML::SaxParser::Callbacks
	end

	def initialize
		super
		@content=Hash.new{|h,k|h[k]=[]}
		@current_tag=[]
	end
	attr_reader :content

	def tag_start(tag,attrs)
		@current_tag.push(tag)
	end
	def start_element(uri,tag,qname,attrs) tag_start(tag,attrs) end
	alias_method :on_start_element, :tag_start
	def tag_end(tag)
		if (t=@current_tag.pop)!=tag then raise "xml is malformed /#{t}" end
	end
	def end_element(uri,tag,qname) tag_end(tag) end
	alias_method :on_end_element, :tag_end
	def cdata(text)
	end
	alias_method :on_cdata_block, :cdata
	def text(text)
		#if @current_tag[0..3]==['dat','games','game','files'] && ['romCRC'].find{|e|e==@current_tag[4]}
		if @current_tag[2..3]==['game','files'] && ['romCRC'].find{|e|e==@current_tag[4]}
			@content[@current_tag[4]]<<text
		end
		if @current_tag[2..2]==['game'] && ['serial'].find{|e|e==@current_tag[3]}
			text=~/^.+-(.+)-.+$/;
			@content[@current_tag[3]]<<$1
		end
	end
	alias_method :characters, :text
	alias_method :on_characters, :text
end

STDERR.puts "Phase4: Parse ADVANsCEne_NDScrc.xml";
listener=GameListListener.new
case SAX
	when 0 then REXML::Parsers::StreamParser.new(xml,listener).parse
	when 1 then parser=REXML::Parsers::SAX2Parser.new(xml);parser.listen(listener);parser.parse
	when 2 then parser=LibXML::XML::SaxParser.string(xml);parser.callbacks=listener;parser.parse
end

STDERR.puts "Phase5: Modify #{gamelist_name}";
crc=listener.content['romCRC']
serial=listener.content['serial']
crc.length.times{|i|
	cont.sub!("[#{crc[i]}]","[#{crc[i]}]{#{serial[i]}}")
}
open(gamelist_name,'wb'){|f|
	f.write(cont)
}
STDERR.puts "Done.";
