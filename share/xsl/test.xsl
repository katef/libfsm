<?xml version="1.0"?>

<xsl:stylesheet version="1.0"
	xmlns="http://www.w3.org/1999/xhtml"
	xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

	<xsl:import href="base.xsl"/>
	<xsl:import href="output.xsl"/>


	<xsl:template name="static-content">
		<table>
			<thead>
				<tr>
					<th>
						<xsl:text>Input</xsl:text>
					</th>
					<th>
						<xsl:text>Expected&#xA0;Output</xsl:text>
					</th>
					<th>
						<xsl:text>Actual&#xA0;Output</xsl:text>
					</th>
				</tr>
			</thead>

			<tbody>
				<xsl:apply-templates select="test"/>
			</tbody>
		</table>
	</xsl:template>

	<xsl:template match="test">
		<tr>
			<td>
				<img src="{@input}"/>
			</td>
			<td>
				<img src="{@expected}"/>
			</td>
			<td>
				<img src="{@actual}"/>
			</td>
		</tr>
	</xsl:template>

	<!-- TODO: make sure xml:output stuff matches that for output.xsl -->
	<xsl:template match="/tests">
		<xsl:call-template name="output-content">
			<xsl:with-param name="css" select="concat(
					' ', $libfsm.url.www, '/css/test.css')"/>

			<xsl:with-param name="title">
				<xsl:value-of select="$test.title"/>
			</xsl:with-param>

			<xsl:with-param name="content.body">
				<h1>
					<xsl:value-of select="$test.title"/>
				</h1>

				<xsl:call-template name="static-content"/>
			</xsl:with-param>
		</xsl:call-template>
	</xsl:template>

</xsl:stylesheet>

