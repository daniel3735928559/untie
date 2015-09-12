<?xml version="1.0" encoding="utf-8"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
  <xsl:strip-space  elements="*"/>
  <xsl:include href="items.xsl" />
  <xsl:include href="menu.xsl" />
  <xsl:template match="/"><xsl:apply-templates select="page" /></xsl:template>

  <xsl:template match="page">
    <html><head><title><xsl:value-of select="@title" /></title><link rel="stylesheet" type="text/css" href="style.css" /></head><body>
    <div class="header"><h1><a href="start.xml">Untie</a></h1></div>
    <div class="all">
      <xsl:apply-templates select="document('menu.xml')/menu">
        <xsl:with-param name="current" select="@uri" />
        <xsl:with-param name="expand" select="'always'" />
        <xsl:with-param name="heading" select="'Menu'" />
      </xsl:apply-templates>
      <div class="content">
        <xsl:apply-templates />
      </div>
    </div>
    </body></html>
  </xsl:template>

  <xsl:template match="red">
    <font color="#c00"><i><xsl:apply-templates select="./text()|./*" /></i></font>
  </xsl:template>

  <xsl:template match="cmd">
    <a class="pre" href="commands.xml#{@name}"><xsl:value-of select="@name" /></a>
  </xsl:template>

  <xsl:template match="code">
    <div class="codebox">
      <pre><xsl:apply-templates select="./text()|./*" /></pre>
    </div>
  </xsl:template>

  <xsl:template match="file">
    <a href="files.xml#{@name}"><xsl:value-of select="@name" /></a>
  </xsl:template>  

  <xsl:template match="@*|node()">
    <xsl:copy>
      <xsl:apply-templates select="@*|node()" />
    </xsl:copy>
  </xsl:template>

</xsl:stylesheet>
